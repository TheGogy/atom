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

    // TODO: Move this to history struct

    for (bool inCheck : {false, true})
        for (Movepicker::StatsType c : {Movepicker::NoCaptures, Movepicker::Captures})
            for (auto& to : continuationHist[inCheck][c])
                for (auto& h : to)
                    h->fill(-427);

    butterflyHist.fill(61);
    captureHist.fill(-598);
    pawnHist.fill(-1188);
    correctionHist.fill(0);

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

    const RootMove& bestMove = bestWorker.rootMoves[0];
    const Position& rootPos  = bestWorker.rootPosition;

    std::string pv;
    for (const Move m : bestMove.pv) {
        pv += Uci::formatMove(m) + " ";
    }

    // Remove last whitespace
    if (!pv.empty()) pv.pop_back();

    SearchInfo info;

    info.depth         = depth;
    info.selDepth      = bestMove.selDepth;
    info.score         = Uci::formatScore(bestMove.uciScore, rootPos);
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
    while (limits.isInfinite && !threads.shouldStop) 
        {}

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


template <Color Me>
void SearchWorker::iterativeDeepening() {

    Value bestScore = -VALUE_INFINITE;
    Value alpha, beta;
    Value delta, avg;

    Move bestPV[MAX_PLY + 1];

    StackObject stack[MAX_PLY + 10] = {};
    StackObject* sPtr = stack + 7;

    // Set up stack ply
    for (int i = 0; i <= MAX_PLY + 2; ++i) {
        (sPtr + i)->ply = i;
    }

    // Set up continuationHist
    for (int i = 7; i > 0; --i) {
        (sPtr - i)->continuationHist = &this->continuationHist[0][0][NO_PIECE][0];
        (sPtr - i)->staticEval = VALUE_NONE;
    }

    sPtr->pv = bestPV;

    // Main iterative deepening loop
    while (++rootDepth < MAX_PLY && !threads.shouldStop
        && !(limits.depth && rootDepth > limits.depth && isFirstThread())) {

        // Save the last iteration's scores for better
        // move ordering
        for (RootMove& rm : rootMoves) {
            rm.prevScore = rm.score;
        }

        // Reset selDepth
        selDepth = 0;

        // Reset aspiration window
        avg = rootMoves[0].avgScore;
        delta = Tunables::ASPIRATION_WINDOW_SIZE + std::abs(rootMoves[0].meanSquaredScore) / Tunables::ASPIRATION_WINDOW_DIVISOR;
        alpha = std::max(-VALUE_INFINITE, avg - delta);
        beta  = std::min( VALUE_INFINITE, avg + delta);

        // Set optimism
        optimism[Me]  = Tunables::OPTIMISM_RATIO_NUMERATOR * avg / (std::abs(avg) + Tunables::OPTIMISM_RATIO_DENOMINATOR);
        optimism[~Me] = -optimism[Me];

        int failedHigh = 0;
        while (true) {

            rootDelta  = beta - alpha;
            bestScore  = pvSearch<Me, NODETYPE_ROOT>(rootPosition, sPtr, alpha, beta, std::max(1, rootDepth - failedHigh), false);

            // Sort moves such that we search the best move first (highest score -> lowest score)
            std::stable_sort(rootMoves.begin(), rootMoves.end());

            // If search has been stopped externally, break immediately.
            if (threads.shouldStop) {
                break;
            }

            // fail low
            if (bestScore <= alpha) {
                beta = (alpha + beta) / 2;
                alpha = std::max(-VALUE_INFINITE, bestScore - delta);
                failedHigh = 0;
            }

            // Fail high
            else if (bestScore >= beta) {
                beta = std::min(VALUE_INFINITE, bestScore + delta);
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
            onNewPv(*this, threads, tt, rootDepth);
        }

        if (threads.shouldStop) {
            break;
        }

        completedDepth = rootDepth;

        if (!limits.isInfinite && !threads.shouldStop) {
            // TODO: Time management
        }
    }
}


// TODO: Add CutNode to template?
template <Color Me, NodeType NT>
Value SearchWorker::pvSearch(
    Position& pos, StackObject* sPtr, Value alpha, Value beta, Depth depth, bool cutNode
) {
    constexpr bool PvNode   = (NT != NODETYPE_NON_PV);
    constexpr bool RootNode = (NT == NODETYPE_ROOT);
    constexpr NodeType QNodeType = (PvNode ? NODETYPE_PV : NODETYPE_NON_PV);

    // Quiescense search at depth 0
    if (depth <= 0) {
        return qSearch<Me, QNodeType>(pos, sPtr, alpha, beta, 0);
    }

    // TODO: Time management should quickly check time here

    // Ensure depth does not exceed max ply
    depth = std::min(depth, MAX_PLY - 1);

    assert(-VALUE_INFINITE <= alpha && alpha < beta && beta <= VALUE_INFINITE);
    assert(PvNode || (alpha == beta - 1));
    assert(0 < depth && depth < MAX_PLY);
    assert(!(PvNode && cutNode));
    assert(0 <= sPtr->ply && sPtr->ply < MAX_PLY);

    // Make sure the depth does not go higher than max ply
    depth = std::min(depth, MAX_PLY - 1);

    SearchWorker* thisThread = this;

    Move pv[MAX_PLY + 1];
    Move currentMove, bestMove = MOVE_NONE;
    Value bestScore = -VALUE_INFINITE;
    Value maxScore  =  VALUE_INFINITE;

    bool improving, oppWorsening;
    Value eval, rawEval = VALUE_NONE;

    PartialMoveList capturesSearched;
    PartialMoveList quietsSearched;

    sPtr->inCheck        = pos.inCheck();
    sPtr->nMoves         = 0;

    // update selDepth
    if (PvNode && selDepth < sPtr->ply + 1) {
        thisThread->selDepth = sPtr->ply + 1;
    }

    if constexpr (!RootNode) {
        // See if search has been aborted
        if (threads.shouldStop.load(std::memory_order_relaxed) || pos.isDraw()) {
            return (sPtr->inCheck && sPtr->ply >= MAX_PLY)
                ? Eval::evaluate<Me>(pos, networks, cacheTable, thisThread->optimism[Me])
                : VALUE_DRAW - 1 + (nodes & 0x2);
        }

        // Mate distance pruning.
        // If we have already found a mate in a shorter distance,
        // don't keep searching: we will not be able to beat that score
        alpha = std::max(alpha, -VALUE_MATE + sPtr->ply);
        beta  = std::min(beta ,  VALUE_MATE - sPtr->ply - 1);
        if (alpha >= beta) return alpha;
    }

    sPtr->statScore      = 0;
    (sPtr + 1)->killer   = MOVE_NONE;

    // Transposition table probe
    auto [ttHit, ttData, ttWriter] = tt.probe(pos.hash());
    sPtr->ttHit = ttHit;

    ttData.move = RootNode ? rootMoves[0].pv[0]
                  : ttHit  ? ttData.move
                           : MOVE_NONE;

    ttData.score = ttHit ? ttData.getAdjustedScore(sPtr->ply) : VALUE_NONE;

    // Transposition table cutoff
    if (!PvNode && ttHit && ttData.depth > depth - (ttData.score <= beta) &&
        ttData.score != VALUE_NONE &&
        ttData.bound & (ttData.score >= beta ? BOUND_LOWER : BOUND_UPPER)) {
      return ttData.score;
    }


    // TODO: Endgame tablebase probe goes here


    if (!sPtr->inCheck) {
        if (ttHit) {
            rawEval = (ttData.eval != VALUE_NONE ? ttData.eval : Eval::evaluate<Me>(pos, networks, cacheTable, thisThread->optimism[Me]));

            if (PvNode && ttData.eval != VALUE_NONE) {
                NNUE::hint_common_parent_position(pos, networks, cacheTable);
            }

            sPtr->staticEval = eval = correctStaticEval<Me>(rawEval, pos);

            if (ttData.score != VALUE_NONE && ttData.bound & (ttData.score > eval ? BOUND_LOWER : BOUND_UPPER)) {
                eval = ttData.score;
            }

        } else {
            rawEval = Eval::evaluate<Me>(pos, networks, cacheTable, thisThread->optimism[Me]);

            sPtr->staticEval = eval = correctStaticEval<Me>(rawEval, pos);

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
            Value score = qSearch<Me, NODETYPE_NON_PV>(pos, sPtr, alpha - 1, alpha, 0);
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

            assert(eval - beta >= 0);

            Depth R = getNullMoveReductionAmount(eval, beta, depth);
            sPtr->currentMove = MOVE_NULL;
            sPtr->continuationHist = &thisThread->continuationHist[0][0][NO_PIECE][0];

            pos.doNullMove<Me>(tt);
            Value nullSearchScore = -pvSearch<~Me, NODETYPE_NON_PV>(pos, sPtr + 1, -beta, -beta + 1, depth - R, false);
            pos.undoNullMove<Me>();

            if (nullSearchScore >= beta && nullSearchScore < VALUE_TB_WIN_IN_MAX_PLY) {

                // Ensure we don't run verification search too often
                if (thisThread->nmpCutoff || depth < Tunables::NMP_VERIFICATION_MIN_DEPTH) {
                    return nullSearchScore;
                }

                assert(!thisThread->nmpCutoff);

                thisThread->nmpCutoff = sPtr->ply + Tunables::NMP_DEPTH_SCALE * (depth - R) / Tunables::NMP_DEPTH_DIVISOR;

                // Verification search
                Value v = pvSearch<Me, NODETYPE_NON_PV>(pos, sPtr, beta - 1, beta, depth - R, false);

                thisThread->nmpCutoff = 0;

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
            return qSearch<Me, NODETYPE_PV>(pos, sPtr, alpha, beta, 0);
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

    const Movepicker::PieceToHistory* tempContHist[] = {
        (sPtr - 1)->continuationHist,
        (sPtr - 2)->continuationHist,
        (sPtr - 3)->continuationHist,
        (sPtr - 4)->continuationHist,
        nullptr,
        (sPtr - 6)->continuationHist
    };

    Movepicker::MovePicker<Me> mp(pos, ttData.move, sPtr->killer, depth,
                                  &thisThread->butterflyHist,
                                  &thisThread->captureHist,
                                  tempContHist,
                                  &thisThread->pawnHist
                                  );

    int nMoves = 0;
    bool givesCheck, isCapture;

    bool skipQuiet = false;

    Value score, delta;
    Depth newDepth, reduction;
    Piece movedPiece;

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

        if (RootNode && isFirstThread() && nodes > 1000000) {
            Uci::callbackIter(depth, currentMove, nMoves);
        }

        if constexpr (PvNode) {
            (sPtr + 1)->pv = nullptr;
        }

        givesCheck = pos.givesCheck<Me>(currentMove);
        isCapture  = pos.getPieceAt(moveTo(currentMove)) != NO_PIECE;
        movedPiece = pos.getPieceAt(moveFrom(currentMove));

        assert(movedPiece != NO_PIECE);

        newDepth = depth - 1;

        delta = beta - alpha;
        reduction = getReduction(improving, depth, nMoves, delta);

        // Late move pruning
        if (!RootNode && bestScore > VALUE_TB_LOSS_IN_MAX_PLY && pos.hasNonPawnMaterial<Me>()) {

            skipQuiet = (nMoves >= ((3 + depth * depth) / (2 - improving)));

            int lmpDepth = newDepth - reduction;

            if (isCapture || givesCheck) {
                Piece capturedPiece = pos.getPieceAt(moveTo(currentMove));
                int captHist = thisThread->captureHist[movedPiece][moveTo(currentMove)][typeOf(capturedPiece)];

                // Futility pruning
                if (!givesCheck
                    && lmpDepth < Tunables::FUTULITY_PRUNING_CAPTURE_MAX_DEPTH 
                    && !sPtr->inCheck
                    && (
                        sPtr->staticEval + Tunables::FUTILITY_PRUNING_CAPTURE_BASE
                        + lmpDepth * Tunables::FUTILITY_PRUNING_CAPTURE_LMPDEPTH_SCALE
                        + PIECE_VALUE[capturedPiece]
                        + captHist / Tunables::FUTILITY_PRUNING_CAPT_HIST_SCALE
                    ) <= alpha
                ) {
                    continue;
                }

                // SEE pruning
                int seeHist = std::clamp(
                    captHist / Tunables::FUTILITY_PRUNING_SEE_HISTORY_NORMALIZER,
                    depth * -Tunables::FUTILITY_PRUNING_SEE_DEPTH_SCALE_MIN,
                    depth *  Tunables::FUTILITY_PRUNING_SEE_DEPTH_SCALE_MAX
                );
                if (!pos.see(currentMove, Tunables::FUTILITY_PRUNING_SEE_DEPTH_SCALE_THRESHOLD * depth - seeHist)) {
                    continue;
                }
            }

            else {
                int history =
                  (*tempContHist[0])[movedPiece][moveTo(currentMove)]
                  + (*tempContHist[1])[movedPiece][moveTo(currentMove)]
                  + thisThread->pawnHist[Movepicker::pawnStructureIndex(pos)][movedPiece][moveTo(currentMove)];

                // Prune some moves if their history is bad
                if (history < depth * Tunables::CONT_HIST_PRUNING_SCALE) {
                    continue;
                }

                history += 2 * thisThread->butterflyHist[Me][moveFromTo(currentMove)];

                // Increase late move pruning depth and ensure it is positive
                lmpDepth += history / Tunables::LMP_DEPTH_HISTORY_SCALE;
                lmpDepth = std::max(lmpDepth, 0);
            }
        }

        // Prefetch TT entry
        tt.prefetch(pos.hashAfter(currentMove));

        sPtr->currentMove = currentMove;
        sPtr->continuationHist = &thisThread->continuationHist[sPtr->inCheck][isCapture][movedPiece][moveTo(currentMove)];

        // Increment nodes
        thisThread->nodes.fetch_add(1, std::memory_order_relaxed);

        // Make the move
        pos.doMove<Me>(currentMove);

        // Decrease reduction
        if (sPtr->ttPv) {
            reduction -= (1 + (ttData.score > alpha) + (ttData.depth >= depth));
        }

        if constexpr (PvNode) {
            --reduction;
        }

        // Increase reduction
        if (cutNode) {
            reduction += 2 - (ttData.depth >= depth && sPtr->ttPv)
                      + (!sPtr->ttPv && currentMove != ttData.move && currentMove != sPtr->killer);
        }

        if (ttData.move && pos.isTactical(ttData.move)) {
            ++reduction;
        }

        sPtr->statScore = 2 * thisThread->butterflyHist[Me][moveFromTo(currentMove)]
                        + (*tempContHist[0])[movedPiece][moveTo(currentMove)]
                        + (*tempContHist[1])[movedPiece][moveTo(currentMove)]
                        - Tunables::STAT_SCORE_HISTORY_REDUCTION;

        reduction -= sPtr->statScore / Tunables::REDUCTION_STAT_SCORE_NORMALIZER;

        // Late moves reduction
        if (depth >= 2 && nMoves > 1 + (RootNode && depth < 10)) {

            Depth d = std::max(1, std::min(newDepth - reduction, newDepth + 1));

            // Narrow search with reduced depth
            score = -pvSearch<~Me, NODETYPE_NON_PV>(pos, sPtr + 1, -alpha - 1, -alpha, d, true);

            // Do full depth search if LMR fails high
            if (score > alpha && d < newDepth) {
                // Narrow search with full depth
                score = -pvSearch<~Me, NODETYPE_NON_PV>(pos, sPtr + 1, -alpha - 1, -alpha, newDepth - 1, !cutNode);
            }

            int bonus = score <= alpha ? -statMalus(newDepth)
                      : score >= beta  ?  statBonus(newDepth)
                                       : 0;

            updateContHistories(sPtr, movedPiece, moveTo(currentMove), bonus);

        // If LMR is skipped, do a full depth search
        } else if (!PvNode || nMoves > 1) {
            if (!ttData.move) reduction += 2;
            score = -pvSearch<~Me, NODETYPE_NON_PV>(pos, sPtr + 1, -alpha - 1, -alpha, newDepth - (reduction > Tunables::REDUCTION_HIGH_THRESHOLD), !cutNode);
        }

        // Full PV search with full window (for PV nodes only)
        if (PvNode && (nMoves == 1 || score > alpha)) {
            (sPtr + 1)->pv = pv;
            (sPtr + 1)->pv[0] = MOVE_NONE;

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
            rm.meanSquaredScore = rm.meanSquaredScore != -VALUE_INFINITE * VALUE_INFINITE
                                ? (score * score + rm.meanSquaredScore) / 2
                                : score * score;

            if (nMoves == 1 || score > alpha) {

                rm.score = rm.uciScore = score;
                rm.selDepth = thisThread->selDepth;

                if (score >= beta) {
                    rm.uciScore = beta;
                }
                else if (score <= alpha) {
                    rm.uciScore = alpha;
                }

                rm.pv.resize(1);
                assert((sPtr + 1)->pv);

                for (Move* m = (sPtr + 1)->pv; *m != MOVE_NONE; ++m) {
                    rm.pv.push_back(*m);
                }

                // TODO: Best move changes

            } else {
                // All non-pv moves are set to -inf.
                // This is not a problem: the move sorting is stable
                // and so the order is preserved.
                rm.score = -VALUE_INFINITE;
            }
        }

        if (score > bestScore) {
            bestScore = score;

            if (score > alpha) {
                bestMove = currentMove;

                if constexpr (PvNode && !RootNode) {
                    updatePv(sPtr->pv, currentMove, (sPtr + 1)->pv);
                }

                if (score >= beta) {
                    break;
                }

                else {
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

        // if this move is not the best move, update its stats so it is not searched
        // first in future searches
        if (currentMove != bestMove && nMoves <= 32) {
            if (isCapture) capturesSearched.push_back(currentMove);
            else           quietsSearched.push_back(currentMove);
        }
    }

    assert (nMoves || sPtr->inCheck || Movegen::countLegalMoves<Me>(pos) == 0);

    // If there are no moves, we are in checkmate / stalemate
    if (!nMoves) {
        bestScore = sPtr->inCheck ? -VALUE_MATE + sPtr->ply : VALUE_DRAW;
    }

    if (bestMove != MOVE_NONE) {
        updateAllHistories<Me>(pos, sPtr, bestMove, quietsSearched, capturesSearched, depth);
    }

    if constexpr (PvNode) {
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
        sPtr->ttPv,
        bestMove,
        tt.getAge(),
        bestScore >= beta ? BOUND_LOWER
            : PvNode && bestMove ? BOUND_EXACT
                                 : BOUND_UPPER
    );

    // Update correction history
    if (
        !sPtr->inCheck
     && !(bestMove && pos.isTactical(bestMove))
     && !(bestScore >= beta && bestScore <= sPtr->staticEval)
     && !(!bestMove && bestScore >= sPtr->staticEval)
    ) {
        Value bonus = std::clamp(int(bestScore - sPtr->staticEval) * depth / 8,
                                -Movepicker::CORRECTION_HISTORY_LIMIT / 4, Movepicker::CORRECTION_HISTORY_LIMIT / 4);

        thisThread->correctionHist[Me][Movepicker::pawnStructureIndex<Movepicker::PH_CORRECTION>(pos)] << bonus;
    }

    assert(bestScore > -VALUE_INFINITE && bestScore < VALUE_INFINITE);

    return bestScore;

}


// Quiescense search function, called by main search with depth 0.
// This recursively searches moves on the search horizon, to ensure
// that the static evaluation is not confused by tactical moves and misjudges
// a capture.
template <Color Me, NodeType NT>
Value SearchWorker::qSearch(
    Position& pos, StackObject* sPtr, Value alpha, Value beta, Depth depth
) {

    constexpr bool PvNode = NT == NODETYPE_PV;

    static_assert(NT != NODETYPE_ROOT);
    assert(depth <= 0);
    assert(alpha >= -VALUE_INFINITE && alpha < beta && beta <= VALUE_INFINITE);
    assert(PvNode || (alpha == beta - 1));

    SearchWorker* thisThread = this;

    Move pv[MAX_PLY + 1];

    Value score, bestScore, rawEval = VALUE_NONE;
    Value futilityBase;
    Move currentMove, bestMove = MOVE_NONE;
    int nMoves = 0;

    sPtr->inCheck = pos.inCheck();

    if constexpr (PvNode) {
        (sPtr + 1)->pv = pv;
        sPtr->pv[0]    = MOVE_NONE;
    }

    // Update selDepth
    if (PvNode && thisThread->selDepth < sPtr->ply + 1) {
        thisThread->selDepth = sPtr->ply + 1;
    }

    // Check for draw or if we have reached MAX PLY
    if (pos.isDraw() || sPtr->ply >= MAX_PLY) {
        return (sPtr->ply >= MAX_PLY && !sPtr->inCheck)
            ? Eval::evaluate<Me>(pos, networks, cacheTable, thisThread->optimism[Me])
            : VALUE_DRAW;
    }

    assert(0 <= sPtr->ply && sPtr->ply < MAX_PLY);

    auto [ttHit, ttData, ttWriter] = tt.probe(pos.hash());
    sPtr->ttHit  = ttHit;
    ttData.move  = ttHit ? ttData.move : MOVE_NONE;
    ttData.score = ttHit ? ttData.getAdjustedScore(sPtr->ply) : VALUE_NONE;

    Depth qsTtDepth = sPtr->inCheck || depth >= 0 ? 0 : -1;

    // Check for if we can cut off using the transposition table score
    if(
        !PvNode && ttData.score != VALUE_NONE
     && (ttData.depth >= qsTtDepth)
     && (ttData.bound & (ttData.score >= beta ? BOUND_LOWER : BOUND_UPPER))
    ) {
        return ttData.score;
    }

    // Static eval of position
    if (!sPtr->inCheck) {
        if (sPtr->ttHit) {
            rawEval = (ttData.eval != VALUE_NONE ? ttData.eval : Eval::evaluate<Me>(pos, networks, cacheTable, thisThread->optimism[Me]));
            sPtr->staticEval = bestScore = correctStaticEval<Me>(rawEval, pos);

            // Use value from tt if possible
            if (std::abs(ttData.score) < VALUE_TB_WIN_IN_MAX_PLY
            && (ttData.bound & (ttData.score > bestScore ? BOUND_LOWER : BOUND_UPPER))
            ) {
                bestScore = ttData.score;
            }

        } else {

            rawEval = (sPtr - 1)->currentMove != MOVE_NULL
                    ? Eval::evaluate<Me>(pos, networks, cacheTable, thisThread->optimism[Me])
                    : -(sPtr - 1)->staticEval;

            sPtr->staticEval = bestScore = correctStaticEval<Me>(rawEval, pos);
        }

        // If static eval is at least beta, return immediately
        if (bestScore >= beta) {
            if (std::abs(bestScore) < VALUE_TB_WIN_IN_MAX_PLY && !PvNode) {
                bestScore = (beta + 3 * bestScore) / 4;
            }

            // Write to transposition table
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

        // Update alpha
        if (bestScore > alpha) {
            alpha = bestScore;
        }

        futilityBase = sPtr->staticEval + Tunables::FUTILITY_BASE_INCREMENT;

    } else {
        bestScore = futilityBase = -VALUE_INFINITE;
    }


    const Movepicker::PieceToHistory* tempContHist[] = {
        (sPtr - 1)->continuationHist,
        (sPtr - 2)->continuationHist
    };

    Movepicker::MovePicker<Me> mp(pos, ttData.move, sPtr->killer, depth,
                                  &thisThread->butterflyHist,
                                  &thisThread->captureHist,
                                  tempContHist,
                                  &thisThread->pawnHist
                                  );

    Square prevSquare = (isNotNullMove((sPtr - 1)->currentMove)) ? (moveTo((sPtr - 1)->currentMove)) : SQ_NONE;

    bool givesCheck, isTactical;

    while ((currentMove = mp.nextMove()) != MOVE_NONE) {
        assert(isValidMove(currentMove));

        ++nMoves;

        givesCheck = pos.givesCheck<Me>(currentMove);
        isTactical = pos.isTactical(currentMove);

        // Pruning
        if (bestScore > VALUE_TB_LOSS_IN_MAX_PLY
         && pos.hasNonPawnMaterial<Me>()
        ) {

            if (
                !givesCheck
             && moveTo(currentMove) != prevSquare
             && futilityBase > VALUE_TB_LOSS_IN_MAX_PLY
             && moveTypeOf(currentMove) != MT_PROMOTION
            ) {

                // Move count pruning
                if (nMoves > 2) {
                    continue;
                }

                Value futility = futilityBase + PIECE_VALUE[pos.getPieceAt(moveTo(currentMove))];

                // If static eval + value of piece we are going to capture
                // is much lower than alpha, prune this move
                if (futility <= alpha) {
                    bestScore = std::max(bestScore, futility);
                    continue;
                }

                // If static eval is worse than alpha and we don't win material, prune this move
                if (futilityBase <= alpha && !pos.see(currentMove, 1)) {
                    bestScore = std::max(bestScore, futilityBase);
                    continue;
                }

                // If SEE eval is much worse than the alpha cutoff, we can prune this move
                if (futilityBase > alpha && !pos.see(currentMove, (alpha - futilityBase) * Tunables::FUTILITY_SEE_PRUNING_MULTIPLIER)) {
                    bestScore = alpha;
                    continue;
                }
            }

            // History based pruning
            if (!isTactical && (
                    (*tempContHist[0])[pos.getPieceAt(moveFrom(currentMove))][moveTo(currentMove)]
                  + (*tempContHist[1])[pos.getPieceAt(moveFrom(currentMove))][moveTo(currentMove)]
                  + thisThread->pawnHist[Movepicker::pawnStructureIndex(pos)][pos.getPieceAt(moveFrom(currentMove))][moveTo(currentMove)]
                <= Tunables::CONT_HIST_PRUNNING_THRESHOLD
                )
            ) {
                continue;
            }

            // SEE pruning
            if (!pos.see(currentMove, -83)) {
                continue;
            }
        }

        // Prefetch probable tt entry early
        tt.prefetch(pos.hashAfter(currentMove));

        sPtr->currentMove = currentMove;
        sPtr->continuationHist = &thisThread->continuationHist[sPtr->inCheck][pos.isTactical(currentMove)][pos.getPieceAt(moveFrom(currentMove))][moveTo(currentMove)];

        // Increment nodes
        thisThread->nodes.fetch_add(1, std::memory_order_relaxed);

        // Recursive part
        pos.doMove<Me>(currentMove);
        score = -qSearch<~Me, NT>(pos, sPtr + 1, -beta, -alpha, depth - 1);
        pos.undoMove<Me>(currentMove);

        assert(score > -VALUE_INFINITE && score < VALUE_INFINITE);

        // See if move is better than our current best move
        if (score > bestScore) {
            bestScore = score;

            if (score > alpha) {
                bestMove = currentMove;

                // Update the PV
                if constexpr (PvNode) {
                    updatePv(sPtr->pv, currentMove, (sPtr + 1)->pv);
                }

                // Update alpha
                if (score < beta) {
                    alpha = score;
                }

                // Fail high
                else {
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
        rawEval,
        qsTtDepth,
        ttHit && ttData.isPv,
        bestMove,
        tt.getAge(),
        bestScore >= beta ? BOUND_LOWER : BOUND_UPPER
    );

    assert(bestScore > -VALUE_INFINITE && bestScore < VALUE_INFINITE);

    return bestScore;
}


} // namespace search

} // namespace Atom
