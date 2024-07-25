#include <cstdint>

#include "search.h"
#include "movegen.h"
#include "position.h"
#include "thread.h"
#include "tt.h"
#include "types.h"
#include "uci.h"

namespace Atom {

namespace Search {


void SearchWorker::clear() {
    cacheTable.clear(networks);
}


// This should only be called by the main thread (id 0)
void SearchWorker::onNewPv(
    SearchWorker bestWorker,
    const ThreadPool& threads,
    const TranspositionTable tt,
    Depth depth
) {
    const uint64_t totalNodesSearched = threads.totalNodesSearched();
    const uint64_t totalTbHits        = threads.totalTbHits();

    const RootMoveList& rootMoves = bestWorker.rootMoves;
    const Position&     rootPos   = bestWorker.rootPosition;

    const size_t idx = bestWorker.idx;

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

    if (rootMoves.isEmpty()) {
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
        // TODO: Add SearchWorker::onNewPv here
        // threads.firstWorker.onNewPv(...)
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
    MoveList bestPV;
    Value bestValue = -VALUE_INFINITE, prevBestValue = -VALUE_INFINITE;
    Value alpha, beta;

}


} // namespace search

} // namespace Atom
