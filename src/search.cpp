#include <algorithm>
#include <cstdint>

#include "search.h"
#include "movegen.h"
#include "position.h"
#include "thread.h"
#include "tt.h"
#include "tunables.h"
#include "types.h"
#include "uci.h"

namespace Atom {

namespace Search {


void SearchWorker::clear() {
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
    info.pv            = pv;
}


void SearchWorker::startSearch() {

    // All threads except the first one go straight to searching
    if (!isFirstThread()) {
        iterativeDeepening();
        return;
    }

    // We only want the following code to be run once:
    // it will only be run by the first thread.
    tt.onNewSearch();

    if (rootMoves.empty()) {
        rootMoves.push_back(Move::MOVE_NONE);
    } else {
        // Start all the other threads going
        threads.startSearch();
        iterativeDeepening();
    }

    // If the search is infinite, wait here and do nothing.
    while (isInfinite && !threads.shouldStop) {}

    // Wait for the other threads to stop
    threads.shouldStop = true;
    threads.waitForFinish();

    SearchWorker* bestWorker = threads.bestThread()->worker.get();

    threads.firstWorker()->bestPrevScore = bestWorker->rootMoves[0].score;

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
    Value bestValue = -VALUE_INFINITE, prevBestValue = -VALUE_INFINITE;
    Value alpha, beta, delta, avg;

    Move bestPV[MAX_PLY + 1];
    MoveList prevBestPV;

    StackObject stack[MAX_PLY];
    StackObject* sPtr = stack;

    Depth searchUpTo;

    sPtr->pv = bestPV;

    // Main iterative deepening loop
    while (++searchDepth < maxDepth && !threads.shouldStop && searchDepth < MAX_PLY) {

        // Save the scores from the previous iteration. This is what allows
        // us to be fast with iterative deepening searches.
        for (RootMove& rm : rootMoves) {
            rm.prevScore = rm.score;
        }

        // Reset selDepth
        selDepth = 0;

        // Reset aspiration window
        avg = rootMoves[0].avgScore;
        delta = Tunables::ASPIRATION_WINDOW_SIZE + std::abs(avg) / Tunables::ASPIRATION_WINDOW_DIVISOR;
        alpha = std::max(-VALUE_INFINITE, avg - delta);
        beta  = std::min( VALUE_INFINITE, avg + delta);

        searchUpTo = searchDepth;

        while (true) {

            bestValue = pvSearch<Me, NODETYPE_ROOT>(rootPosition, sPtr, alpha, beta, currentDepth, false);

            // Sort moves such that we search the best move first (highest score -> lowest score)
            std::stable_sort(rootMoves.begin(), rootMoves.end());

            if (threads.shouldStop) break;

            // fail low
            if (bestValue <= alpha) {
                beta = (alpha + beta) / 2;
                alpha = std::max(-VALUE_INFINITE, bestValue - delta);
            }

            // Fail high
            else if (bestValue >= beta) {
                beta = std::min(VALUE_INFINITE, bestValue + delta);
                searchDepth -= (std::abs(bestValue) < 1000);
            }

            else {
                break;
            }

            delta += delta / Tunables::DELTA_INCREMENT_DIV;
        }

        // Send update to the GUI
        // Must do this before stopping
        if (isFirstThread() && (nodes > Tunables::UPDATE_NODES || threads.shouldStop)) {
            onNewPv(*this, threads, tt, searchDepth);
        }

        if (threads.shouldStop) break;

        completedDepth = searchDepth;

        prevBestValue = rootMoves[0].score;
        prevBestPV    = rootMoves[0].pv;
    }
}


template<Color Me, NodeType nodeType>
Value SearchWorker::pvSearch(
    Position& pos, StackObject* sPtr, Value alpha, Value beta, Depth depth, bool cutNode
) {
    constexpr bool pvNode   = (nodeType != NODETYPE_NON_PV);
    constexpr bool rootNode = (nodeType == NODETYPE_ROOT);
    constexpr NodeType qNodeType = (pvNode ? NODETYPE_PV : NODETYPE_NON_PV);

    // Quiescense search at depth 0
    if (depth <= 0) {
        return qSearch<Me, qNodeType>(pos, sPtr, alpha, beta, 0);
    }

    // Mate distance pruning
    if constexpr (!rootNode) {
        alpha = std::max(alpha, -VALUE_MATE + sPtr->ply);
        beta  = std::min(beta ,  VALUE_MATE - sPtr->ply - 1);
        if (alpha >= beta) return alpha;
    }

    // Make sure the depth does not go higher than max ply
    depth = std::min(depth, MAX_PLY - 1);

    Move pv[MAX_PLY + 1];
    Move currentMove, bestMove;
    Value bestScore = -VALUE_INFINITE;
    Value maxScore  =  VALUE_INFINITE;

    bool improving, oppWorsening;
    Value eval;
    BoardState tempBoardState;

    SearchWorker* worker = this;
    sPtr->inCheck        = pos.inCheck();
    (sPtr + 1)->killer   = MOVE_NONE;

    // Transposition table probe
    auto [ttHit, ttData, ttWriter] = tt.probe(pos.hash());
    sPtr->ttHit = ttHit;

    ttData.move = rootNode ? rootMoves[0].pv[0]
                  : ttHit  ? ttData.move
                           : MOVE_NONE;

    ttData.score = ttHit ? ttData.getAdjustedScore(sPtr->ply) : VALUE_NONE;

    // Transposition table cutoff
    if (!pvNode && ttHit && ttData.depth > depth - (ttData.score <= beta) &&
        ttData.score != VALUE_NONE &&
        ttData.bound & (ttData.score >= beta ? BOUND_LOWER : BOUND_UPPER)) {
      return ttData.score;
    }

    // TODO: Endgame tablebase probe goes here

    if (sPtr->inCheck) {
        sPtr->staticEval = eval = (sPtr - 2)->staticEval;
        improving = false;

        // TODO: Movepicker

    } else {
        if (ttHit) {
            // TODO: Evaluation
            // sPtr->staticEval = eval = (ttData.eval != VALUE_NONE ? ttData.eval : clampEval(evaluate<Me>(pos)));
            if (ttData.score != VALUE_NONE && ttData.bound & (ttData.score > eval ? BOUND_LOWER : BOUND_UPPER)) {
                eval = ttData.score;
            }
        } else {
            // TODO: Evaluation
            // sPtr->staticEval = eval = clampEval(evaluate<Me>(pos));
            ttWriter.write(pos.hash(), VALUE_NONE, eval, -2, sPtr->ttPv,
                           MOVE_NONE, tt.getAge(), BOUND_NONE);
        }
    }

    // Check if we are improving / opponent is worsening
    improving    = sPtr->staticEval > (sPtr - 2)->staticEval;
    oppWorsening = sPtr->staticEval + (sPtr - 1)->staticEval > 2;

    // Reverse futility pruning (static null move pruning)
    if (!pvNode && !sPtr->inCheck && depth <= Tunables::RFP_DEPTH &&
        eval - (Tunables::RFP_DEPTH_MULTIPLIER * depth) >= beta
    ) {
        return eval;
    }

    // Razoring
    if (!pvNode && !sPtr->inCheck && depth <= Tunables::RAZORING_DEPTH &&
        eval + (Tunables::RAZORING_DEPTH_MULTIPLIER * depth) >= beta
    ) {
        Value score = qSearch<Me, qNodeType>(pos, sPtr, alpha, alpha - 1, 0);
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
    if (cutNode && (sPtr - 1)->currentMove != MOVE_NULL
        && eval >= beta
        && pos.hasNonPawnMaterial<Me>()
        && beta > VALUE_TB_LOSS_IN_MAX_PLY
        && sPtr->ply >= nmpCutoff
    ) {
        Depth R = getNullMoveReductionAmount(eval, beta, depth);
        sPtr->currentMove = MOVE_NULL;

        pos.doNullMove<Me>(tempBoardState, tt);
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
    if (pvNode && !ttData.move) {
        depth -= Tunables::IIR_REDUCTION;
    }

    // Quiescense search if the depth was reduced
    if (depth <= 0) {
        return qSearch<Me, qNodeType>(pos, sPtr, alpha, beta, 0);
    }
}


template<Color Me, NodeType nodeType>
Value SearchWorker::qSearch(
    Position& pos, StackObject* sPtr, Value alpha, Value beta, Depth depth
) {
}

} // namespace search

} // namespace Atom
