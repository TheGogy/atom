#include <atomic>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "search.h"
#include "evaluate.h"
#include "movegen.h"
#include "movepicker.h"
#include "nnue/nnue_misc.h"
#include "position.h"
#include "thread.h"
#include "tt.h"
#include "tunables.h"
#include "types.h"
#include "uci.h"

namespace Atom {

namespace Search {


void SearchWorker::clear() {
    for (size_t i = 1; i < reductions.size(); ++i) {
        reductions[i] = int((Tunables::REDUCTION_AMOUNT + std::log(size_t(threads.size())) / 2) * std::log(i));
    }

    cacheTable.clear(networks);
}


// This should only be called by the main thread (id 0)
void SearchWorker::onNewPv(
    SearchWorker& bestWorker,
    const ThreadPool& threads,
    const TranspositionTable& tt,
    Depth depth
) {
    const uint64_t totalNodesSearched = threads.totalNodesSearched();
    const uint64_t totalTbHits        = threads.totalTbHits();

    const RootMoveList& rootMoves = bestWorker.rootMoves;
    const Position&     rootPos   = bestWorker.rootPosition;

    std::string pv;
    for (const Move m : rootMoves[0].pv) {
        pv += Uci::formatMove(m) + " ";
    }

    SearchInfo info;

    info.depth         = depth;
    info.selDepth      = rootMoves[0].selDepth;
    info.score         = Uci::formatScore(rootMoves[0].score, rootPos);
    info.nodesSearched = totalNodesSearched;
    info.hashFull      = tt.hashfull();
    info.tbHits        = totalTbHits;
    info.timeSearched  = now() - limits.startTimePoint;
    info.pv            = pv;

    Uci::callbackInfo(info);
}


void SearchWorker::startSearch() {

    // All threads except the first one go straight to searching
    if (!isFirstThread()) {
        assert(threads.size() > 1);

        iterativeDeepening();
        return;
    }

    // We only want the following code to be run once:
    // it will only be run by the first thread.
    tt.onNewSearch();

    if (!rootMoves.empty()) {
        // Start all the other threads going
        threads.startSearching();
        iterativeDeepening();
    } else {
        // Make sure we have at least something to return
        rootMoves.push_back(Move::MOVE_NONE);
    }

    // If the search is infinite, wait here and do nothing.
    while (limits.isInfinite && !threads.shouldStop) {}

    // Wait for the other threads to stop
    threads.shouldStop = true;
    threads.waitForFinish();

    SearchWorker* bestWorker = threads.bestThread()->worker.get();

    if (bestWorker != this) {
        threads.firstWorker()->onNewPv(*bestWorker, threads, tt, bestWorker->completedDepth);
    }

    std::string bestmove = Uci::formatMove(bestWorker->rootMoves[0].pv[0]);
    std::string ponder;
    if (bestWorker->rootMoves[0].pv.size() > 1) {
        ponder = Uci::formatMove(bestWorker->rootMoves[0].pv[1]);
    }

    Uci::callbackBestMove(bestmove, ponder);
}


template<Color Me>
void SearchWorker::iterativeDeepening() {

    Value bestValue = -VALUE_INFINITE, lastBestValue = -VALUE_INFINITE;
    Value alpha = -VALUE_INFINITE, beta = VALUE_INFINITE;
    Value delta, avg;

    Move bestPV[MAX_PLY + 1];
    MoveList lastBestPV;

    StackObject stack[MAX_PLY + 2];
    StackObject* sPtr = stack;

    // Set up stack ply
    for (int i = 0; i < MAX_PLY + 2; ++i) {
        (sPtr + i)->ply = i;
    }

    Depth searchUpTo;

    sPtr->pv = bestPV;

    // Main iterative deepening loop
    while (++searchDepth < MAX_PLY && !threads.shouldStop
        && !(limits.depth && searchDepth > limits.depth && isFirstThread())) {

        // Reset selDepth
        selDepth = 0;

        // Reset aspiration window
        avg = rootMoves[0].avgScore;
        delta = Tunables::ASPIRATION_WINDOW_SIZE + avg * avg / Tunables::ASPIRATION_WINDOW_DIVISOR;
        alpha = std::max(-VALUE_INFINITE, avg - delta);
        beta  = std::min( VALUE_INFINITE, avg + delta);

        int failedHigh = 0;
        while (true) {

            rootDelta  = beta - alpha;
            searchUpTo = std::max(1, searchDepth - failedHigh);
            bestValue  = pvSearch<Me, NODETYPE_ROOT>(rootPosition, sPtr, alpha, beta, searchUpTo, false);

            // Sort moves such that we search the best move first (highest score -> lowest score)
            std::stable_sort(rootMoves.begin(), rootMoves.end());

            if (threads.shouldStop) break;

            if (isFirstThread() && (bestValue <= alpha || bestValue >= beta) && nodes > Tunables::UPDATE_NODES) {
                onNewPv(*this, threads, tt, searchDepth);
            }

            // fail low
            if (bestValue <= alpha) {
                beta = (alpha + beta) / 2;
                alpha = std::max(-VALUE_INFINITE, bestValue - delta);
                failedHigh = 0;
            }

            // Fail high
            else if (bestValue >= beta) {
                beta = std::min(VALUE_INFINITE, bestValue + delta);
               ++failedHigh;
            }

            else {
                break;
            }

            delta += delta / Tunables::DELTA_INCREMENT_DIV;

            assert(alpha >= -VALUE_INFINITE && beta <= VALUE_INFINITE);
        }

        // Send update to the GUI
        // Must do this before stopping
        if (isFirstThread()
        && !(threads.abortSearch && rootMoves[0].uciScore <= VALUE_TB_LOSS_IN_MAX_PLY)
        ) {
            onNewPv(*this, threads, tt, searchDepth);
        }

        if (threads.shouldStop) {
            break;
        }

        completedDepth = searchDepth;

        // If we have had a PV update
        if (rootMoves[0].pv[0] != lastBestPV[0]) {
            lastBestPV    = rootMoves[0].pv;
            lastBestValue = rootMoves[0].score;
        }

        if (!limits.isInfinite && !threads.shouldStop) {
            // TODO: Time management
        }
    }
}


template<Color Me, NodeType nodeType>
Value SearchWorker::pvSearch(
    Position& pos, StackObject* sPtr, Value alpha, Value beta, Depth depth, bool cutNode
) {
    constexpr bool PvNode   = (nodeType != NODETYPE_NON_PV);
    constexpr bool RootNode = (nodeType == NODETYPE_ROOT);
    constexpr NodeType QNodeType = (PvNode ? NODETYPE_PV : NODETYPE_NON_PV);

    // Quiescense search at depth 0
    if (depth <= 0) {
        return qSearch<Me, QNodeType>(pos, sPtr, alpha, beta);
    }

    // TODO: Time management should quickly check time here

    // Ensure depth does not exceed max ply
    depth = std::min(depth, MAX_PLY - 1);

    if constexpr (!RootNode) {
        // See if search has been aborted
        if (threads.shouldStop.load(std::memory_order_relaxed) || pos.isDraw()) {
            return (sPtr->inCheck && sPtr->ply >= MAX_PLY)
                ? Eval::evaluate<Me>(pos, networks, cacheTable)
                : VALUE_DRAW - 1 + (nodes & 0x2);
        }

        // Mate distance pruning.
        // If we have already found a mate in a shorter distance,
        // don't keep searching: we will not be able to beat that score
        alpha = std::max(alpha, -VALUE_MATE + sPtr->ply);
        beta  = std::min(beta ,  VALUE_MATE - sPtr->ply - 1);
        if (alpha >= beta) return alpha;
    }

    assert(-VALUE_INFINITE <= alpha && alpha < beta && beta <= VALUE_INFINITE);
    assert(PvNode || (alpha == beta - 1));
    assert(0 < depth && depth < MAX_PLY);
    assert(!(PvNode && cutNode));
    assert(0 <= sPtr->ply && sPtr->ply < MAX_PLY);

    // Make sure the depth does not go higher than max ply
    depth = std::min(depth, MAX_PLY - 1);

    Move pv[MAX_PLY + 1];
    Move currentMove, bestMove = MOVE_NONE;
    Value bestScore = -VALUE_INFINITE;
    Value maxScore  =  VALUE_INFINITE;

    bool improving, oppWorsening;
    Value eval, rawEval = VALUE_NONE;

    sPtr->inCheck        = pos.inCheck();
    sPtr->nMoves      = 0;
    sPtr->statScore      = 0;
    (sPtr + 1)->killer   = MOVE_NONE;

    // Transposition table probe
    auto [ttHit, ttData, ttWriter] = tt.probe(pos.hash());
    sPtr->ttHit = ttHit;

    ttData.move = RootNode ? rootMoves[0].pv[0]
                  : ttHit  ? ttData.move
                           : MOVE_NONE;

    ttData.score = ttHit ? ttData.getAdjustedScore(sPtr->ply) : VALUE_NONE;

    // update selDepth
    if (PvNode && selDepth < sPtr->ply + 1) {
        selDepth = sPtr->ply + 1;
    }

    // Transposition table cutoff
    if (!PvNode && ttHit && ttData.depth > depth - (ttData.score <= beta) &&
        ttData.score != VALUE_NONE &&
        ttData.bound & (ttData.score >= beta ? BOUND_LOWER : BOUND_UPPER)) {
      return ttData.score;
    }

    // TODO: Endgame tablebase probe goes here


    if (!sPtr->inCheck) {
        if (ttHit) {
            rawEval = (ttData.eval != VALUE_NONE ? ttData.eval : Eval::evaluate<Me>(pos, networks, cacheTable));

            if (PvNode && ttData.eval != VALUE_NONE) {
                NNUE::hint_common_parent_position(pos, networks, cacheTable);
            }

            sPtr->staticEval = eval = clampEval(rawEval);

            if (ttData.score != VALUE_NONE && ttData.bound & (ttData.score > eval ? BOUND_LOWER : BOUND_UPPER)) {
                eval = ttData.score;
            }

        } else {
            rawEval = Eval::evaluate<Me>(pos, networks, cacheTable);
            sPtr->staticEval = eval = clampEval(rawEval);
            ttWriter.write(pos.hash(), VALUE_NONE, rawEval, -2, sPtr->ttPv,
                         MOVE_NONE, tt.getAge(), BOUND_NONE);
        }

        // Check if we are improving / opponent is worsening
        improving    = sPtr->staticEval > (sPtr - 2)->staticEval;
        oppWorsening = sPtr->staticEval + (sPtr - 1)->staticEval > 2;

        // Reverse futility pruning (static null move pruning)
        if (!PvNode && !sPtr->inCheck && depth <= Tunables::RFP_DEPTH &&
            eval - (Tunables::RFP_DEPTH_MULTIPLIER * depth) >= beta
        ) {
            return eval;
        }

        // Razoring
        if (!PvNode && !sPtr->inCheck && depth <= Tunables::RAZORING_DEPTH &&
            eval + (Tunables::RAZORING_DEPTH_MULTIPLIER * depth) >= beta
        ) {
            Value score = qSearch<Me, NODETYPE_NON_PV>(pos, sPtr, alpha - 1, alpha);
            if (score < alpha && std::abs(score) < VALUE_TB_WIN_IN_MAX_PLY) {
                return score;
            }
        }

        // Futility pruning
        if (!sPtr->ttPv && depth < Tunables::FUTILITY_PRUNING_DEPTH
            && eval >= beta
            && eval - futilityMargin(depth, !cutNode && sPtr->ttHit, improving, oppWorsening, (sPtr - 1)->statScore) >= beta
            && beta > VALUE_TB_LOSS_IN_MAX_PLY
            && eval < VALUE_TB_WIN_IN_MAX_PLY
        ) {
            return beta + (eval - beta) / 3;
        }

        // Null move pruning
        if (   cutNode
            && eval >= beta
            && (sPtr - 1)->currentMove != MOVE_NULL
            && (sPtr - 1)->statScore < Tunables::NMP_VERIFICATION_MAX_STATSCORE
            && sPtr->staticEval >= Tunables::NMP_VERIFICATION_MIN_STAT_EVAL_BASE + beta - (Tunables::NMP_VERIFICATION_MIN_STAT_EVAL_DEPTH_SCALE * depth)
            && sPtr->ply >= nmpCutoff
            && pos.hasNonPawnMaterial<Me>()
            && beta > VALUE_TB_LOSS_IN_MAX_PLY
        ) {

            Depth R = getNullMoveReductionAmount(eval, beta, depth);
            sPtr->currentMove = MOVE_NULL;

            pos.doNullMove<Me>(tt);
            Value nullSearchScore = -pvSearch<~Me, NODETYPE_NON_PV>(pos, sPtr + 1, -beta, -beta + 1, depth - R, false);
            pos.undoNullMove<Me>();

            if (nullSearchScore >= beta && nullSearchScore < VALUE_TB_WIN_IN_MAX_PLY) {

                // Ensure we don't run verification search too often
                if (nmpCutoff || depth < Tunables::NMP_VERIFICATION_MIN_DEPTH) {
                    return nullSearchScore;
                }

                nmpCutoff = sPtr->ply + Tunables::NMP_DEPTH_SCALE * (depth - R) / Tunables::NMP_DEPTH_DIVISOR;

                // Verification search
                Value v = pvSearch<Me, NODETYPE_NON_PV>(pos, sPtr, beta - 1, beta, depth - R, false);

                nmpCutoff = 0;

                if (v >= beta) {
                    return nullSearchScore;
                }
            }
        }

        // Internal Iterative Reduction
        if (PvNode && !ttData.move) {
            depth -= Tunables::IIR_REDUCTION;
        }

        // Quiescense search if the depth was reduced
        if (depth <= 0) {
            return qSearch<Me, NODETYPE_PV>(pos, sPtr, alpha, beta);
        }

        // Decrease depth for cutnodes
        if (
            cutNode && depth >= Tunables::CUTNODE_MIN_DEPTH
         && (!ttData.move || ttData.bound == BOUND_UPPER))
        {
            depth -= 1 + !ttData.move;
        }

    } else {
        sPtr->staticEval = eval = (sPtr - 2)->staticEval;
        improving = false;
    }

    Movepicker::MovePicker<Me> mp(pos, ttData.move, sPtr->killer, depth);

    int nMoves = 0;
    int reduction;
    bool givesCheck, isCapture;

    bool skipQuiet = false;

    Value score;
    Depth newDepth;

    score = bestScore;

    // Search all the moves, stopping if beta cutoff occurs
    while ((currentMove = mp.nextMove(skipQuiet)) != MOVE_NONE) {

        assert(isValidMove(currentMove));
        assert(pos.isPseudoLegalMove<Me>(currentMove));

        // At root, obey the searchmoves UCI option and skip any moves that 
        // are not in the searchmoves list
        if (RootNode && !rootMoves.contains(currentMove)) {
            continue;
        }

        sPtr->nMoves = ++nMoves;

        if (RootNode && isFirstThread() && nodes > Tunables::UPDATE_NODES) {
            Uci::callbackIter(depth, currentMove, nMoves);
        }

        givesCheck = pos.givesCheck<Me>(currentMove);
        isCapture  = pos.getPieceAt(moveTo(currentMove)) != NO_PIECE;

        newDepth = depth - 1;

        if (PvNode) {
            (sPtr + 1)->pv = nullptr;
        }

        // Late move pruning
        if (!RootNode && bestScore > VALUE_TB_LOSS_IN_MAX_PLY) {

            skipQuiet = (nMoves >= ((3 + depth * depth) / (2 - improving)));

            // SEE pruning for checks / captures
            if (
                (givesCheck || isCapture)
             && (depth <= Tunables::SEE_PRUNING_MAX_DEPTH)
             && (!pos.see(currentMove, -depth * (isCapture ? Tunables::SEE_PRUNING_CAP_SCORE : Tunables::SEE_PRUNING_CHK_SCORE)))
            ) {
                continue;
            }
        }

        if (PvNode) (sPtr + 1)->pv = nullptr;

        // Prefetch TT entry
        tt.prefetch(pos.hashAfter(currentMove));

        sPtr->currentMove = currentMove;

        // Increment nodes
        nodes.fetch_add(1, std::memory_order_relaxed);

        // Make the move
        pos.doMove<Me>(currentMove);


        // Late move reduction
        reduction = getReduction(improving, depth, nMoves, beta - alpha);

        // Decrease reduction
        reduction -= PvNode;
        reduction -= pos.inCheck();
        reduction -= ttData.isPv;
        reduction -= ttData.score > alpha;
        reduction -= ttData.depth >= depth;

        // Increase reduction
        reduction += cutNode + cutNode;
        reduction += pos.getPieceAt(moveTo(ttData.move)) != NO_PIECE;

        // Late move reduction
        if (depth >= 2 && nMoves > 1 + (RootNode && depth < 10)) {

            Depth d = std::max(1, std::min(newDepth - reduction, newDepth + 1));

            // Narrow search with reduced depth
            score = -pvSearch<~Me, NODETYPE_NON_PV>(pos, sPtr + 1, -alpha - 1, -alpha, d, true);

            if (score > alpha && d < newDepth) {
                // Narrow search with full depth
                score = -pvSearch<~Me, NODETYPE_NON_PV>(pos, sPtr + 1, -alpha - 1, -alpha, newDepth - 1, !cutNode);
            }

        // If LMR is skipped, do a full depth search
        } else if (!PvNode || nMoves > 1) {
            reduction += 2 * !ttData.move;
            score = -pvSearch<~Me, NODETYPE_NON_PV>(pos, sPtr + 1, -alpha - 1, -alpha, newDepth - (reduction > Tunables::REDUCTION_HIGH_THRESHOLD), !cutNode);
        }

        // Full PV search with full window (for PV nodes only)
        if (PvNode && (nMoves == 1 || score > alpha)) {
            (sPtr + 1)->pv = pv;
            (sPtr + 1)->pv[0] = MOVE_NONE;

            // Extend the depth opf all ttmoves
            if (currentMove == ttData.move && sPtr->ply <= currentDepth * 2) {
                newDepth = std::max(newDepth, 1);
            }

            score = -pvSearch<~Me, NODETYPE_PV>(pos, sPtr + 1, -beta, -alpha, newDepth, false);
        }

        // Undo the move
        pos.undoMove<Me>(currentMove);

        assert(score > -VALUE_INFINITE && score < VALUE_INFINITE);

        // Check if the search has been aborted. If it has, this search cannot be
        // trusted fully: just return zero.
        if (threads.shouldStop.load(std::memory_order_relaxed)) {
            return VALUE_ZERO;
        }


        // Check for new best move
        if constexpr (RootNode) {
            RootMove& rm = *std::find(rootMoves.begin(), rootMoves.end(), currentMove);

            rm.avgScore = (rm.avgScore != -VALUE_INFINITE ? (score + rm.avgScore) / 2 : score); 

            if (nMoves == 1 || score > alpha) {
                rm.score = rm.uciScore = score;
                rm.selDepth = selDepth;

                if (score >= beta) {
                    rm.uciScore = beta;
                } else if (score <= alpha) {
                    rm.uciScore = alpha;
                }

                rm.pv.resize(1);
                assert((sPtr + 1)->pv);

                for (Move* m = (sPtr + 1)->pv; *m != MOVE_NONE; ++m) {
                    rm.pv.push_back(*m);
                }

            } else {
                rm.score = -VALUE_INFINITE;
            }
        }

        if (score > bestScore) {
            bestScore = score;

            if (score > alpha) {
                bestMove = currentMove;

                if (PvNode && !RootNode) {
                    updatePv(sPtr->pv, currentMove, (sPtr + 1)->pv);
                }

                if (score >= beta) {
                    break;
                } else {
                    if (
                        (depth > Tunables::SCORE_IMPROVEMENT_DEPTH_MIN)
                     && (depth < Tunables::SCORE_IMPROVEMENT_DEPTH_MAX)
                     && (std::abs(score) < VALUE_TB_WIN_IN_MAX_PLY)
                    ) {
                        depth -= 2;
                    }
                    assert(depth > 0);
                    alpha = score;
                }
            }
        }
    }

    assert (nMoves || sPtr->inCheck || Movegen::countLegalMoves<Me>(pos) == 0);

    // If there are no moves, we are in checkmate / stalemate
    if (!nMoves) {
        bestScore = sPtr->inCheck ? -VALUE_MATE + sPtr->ply : VALUE_DRAW;
    }

    if (PvNode) {
        // Cap the best score
        bestScore = std::min(bestScore, maxScore);
    }

    if (bestScore <= alpha) {
        // Opponent's last move was probably good
        sPtr->ttPv = sPtr->ttPv || ((sPtr - 1)->ttPv && depth > Tunables::PREVIOUS_POS_TTPV_MIN_DEPTH);
    }

    // Update TT
    ttWriter.write(
        pos.hash(),
        valueToTT(bestScore, sPtr->ply),
        rawEval,
        depth,
        ttData.isPv,
        bestMove,
        tt.getAge(),
        bestScore >= beta ? BOUND_LOWER
            : PvNode && bestMove ? BOUND_EXACT
                                 : BOUND_UPPER
    );

    assert(bestScore > -VALUE_INFINITE && bestScore < VALUE_INFINITE);

    return bestScore;

}


// Quiescense search function, called by main search with depth 0.
// This recursively searches moves on the search horizon, to ensure
// that the static evaluation is not confused by tactical moves and misjudges
// a capture.
template<Color Me, NodeType nodeType>
Value SearchWorker::qSearch(
    Position& pos, StackObject* sPtr, Value alpha, Value beta
) {
    constexpr bool PvNode = nodeType == NODETYPE_PV;

    static_assert(nodeType != NODETYPE_ROOT);
    assert(alpha >= -VALUE_INFINITE && alpha < beta && beta <= VALUE_INFINITE);
    assert(PvNode || (alpha == beta - 1));

    Move pv[MAX_PLY + 1];

    Value score, bestScore, rawEval = VALUE_NONE;
    Move currentMove, bestMove = MOVE_NONE;
    int nMoves = 0;

    sPtr->inCheck = pos.inCheck();

    if constexpr (PvNode) {
        (sPtr + 1)->pv = pv;
        sPtr->pv[0] = MOVE_NONE;
    }

    if (PvNode && selDepth < (sPtr->ply + 1)) {
        selDepth = sPtr->ply + 1;
    }

    if (pos.isDraw() || sPtr->ply >= MAX_PLY) {
        return (sPtr->ply >= MAX_PLY && !sPtr->inCheck)
          ? Eval::evaluate<Me>(pos, networks, cacheTable)
          : VALUE_DRAW;
    }

    assert(0 <= sPtr->ply && sPtr->ply < MAX_PLY);

    auto [ttHit, ttData, ttWriter] = tt.probe(pos.hash());
    sPtr->ttHit  = ttHit;
    ttData.move  = ttHit ? ttData.move : MOVE_NONE;
    ttData.score = ttHit ? ttData.getAdjustedScore(sPtr->ply) : VALUE_NONE;

    // Check for TT cutoff
    if(
        !PvNode && ttData.score != VALUE_NONE
     && (ttData.depth >= 0)
     && (ttData.bound & (ttData.score >= beta ? BOUND_LOWER : BOUND_UPPER))
    ) {
        return ttData.score;
    }

    // Static eval of position
    if (!sPtr->inCheck) {
        if (sPtr->ttHit) {
            rawEval = (ttData.eval != VALUE_NONE ? ttData.eval : Eval::evaluate<Me>(pos, networks, cacheTable));
            sPtr->staticEval = bestScore = clampEval(rawEval);

            // Use value from tt if possible
            if (std::abs(ttData.score) < VALUE_TB_WIN_IN_MAX_PLY
            && (ttData.bound & (ttData.score > bestScore ? BOUND_LOWER : BOUND_UPPER))
            ) {
                bestScore = ttData.score;
            }

        } else {

            rawEval = (sPtr - 1)->currentMove != MOVE_NULL
                    ? Eval::evaluate<Me>(pos, networks, cacheTable)
                    : -(sPtr - 1)->staticEval;

            sPtr->staticEval = bestScore = clampEval(rawEval);
        }

        // If static eval is at least beta, return immediately
        if (bestScore >= beta) {
            if (std::abs(bestScore) < VALUE_TB_WIN_IN_MAX_PLY && !PvNode) {
                bestScore = (beta + 3 * bestScore) / 4;
            }

            if (!sPtr->ttHit) {
                ttWriter.write(
                    pos.hash(),
                    valueToTT(bestScore, sPtr->ply),
                    rawEval,
                    -2,
                    false,
                    MOVE_NONE,
                    tt.getAge(),
                    BOUND_LOWER
                );
            }

            return bestScore;
        }

        if (bestScore > alpha) {
            alpha = bestScore;
        }

    } else {
        bestScore = -VALUE_INFINITE;
    }


    Movepicker::MovePicker<Me> mp(pos, ttData.move, sPtr->killer, 0);


    while ((currentMove = mp.nextMove()) != MOVE_NONE) {
        assert(isValidMove(currentMove));

        ++nMoves;

        // Do not search moves with bad SEE score
        if (bestScore > VALUE_TB_LOSS_IN_MAX_PLY
         && pos.hasNonPawnMaterial<Me>()
         && !pos.see(currentMove, 0)
        ) {
            continue;
        }

        // Prefetch probable tt entry early
        tt.prefetch(pos.hashAfter(currentMove));

        sPtr->currentMove = currentMove;

        // Increment nodes
        nodes.fetch_add(1, std::memory_order_relaxed);

        // Recursive part
        pos.doMove<Me>(currentMove);
        score = -qSearch<~Me, nodeType>(pos, sPtr + 1, -beta, -alpha);
        pos.undoMove<Me>(currentMove);

        assert(score > -VALUE_INFINITE && score < VALUE_INFINITE);

        if (score > bestScore) {
            bestScore = score;

            if (score > alpha) {
                bestMove = currentMove;

                // Update the PV
                if (PvNode) updatePv(sPtr->pv, currentMove, (sPtr + 1)->pv);

                // Update alpha
                if (score < beta) {
                    alpha = score;
                } else {
                    // Fail high here
                    break;
                }
            }
        }
    }

    // Check for checkmate (we are in check and the best score has not been changed)
    if (sPtr->inCheck && bestScore == -VALUE_INFINITE) {
        assert(Movegen::countLegalMoves<Me>(pos) == 0);
        return -VALUE_MATE + sPtr->ply;
    }

    if (std::abs(bestScore) < VALUE_TB_WIN_IN_MAX_PLY && bestScore >= beta) {
        bestScore = (3 * bestScore + beta) / 4;
    }

    // Update TT
    ttWriter.write(
        pos.hash(),
        valueToTT(bestScore, sPtr->ply),
        bestScore,
        0,
        ttHit && ttData.isPv,
        bestMove,
        tt.getAge(),
        bestScore > beta ? BOUND_LOWER : BOUND_UPPER
    );

    assert(bestScore > -VALUE_INFINITE && bestScore < VALUE_INFINITE);

    return bestScore;
}


} // namespace search

} // namespace Atom
