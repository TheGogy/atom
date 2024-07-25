#ifndef SEARCH_H
#define SEARCH_H

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string_view>
#include <vector>

#include "movegen.h"
#include "nnue/network.h"
#include "nnue/nnue_accumulator.h"
#include "tt.h"
#include "types.h"

namespace Atom {


using TimePoint = std::chrono::milliseconds::rep;

inline TimePoint now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::high_resolution_clock::now().time_since_epoch())
      .count();
}


// Declare these here: they are defined in thread.h
class ThreadPool;
class TranspositionTable;


namespace Search {

struct SearchInfo {
    int depth;
    int selDepth;
    size_t timeSearched;
    size_t nodesSearched;
    std::string_view pv;
    std::string_view score;
    int hashFull;
    size_t tbHits;
};



struct SearchLimits {
    std::vector<std::string> searchMoves;
    TimePoint time[COLOR_NB], inc[COLOR_NB];
    TimePoint startTimePoint, moveTime;
    bool isInfinite;
    uint64_t nodes;
    int depth, mate, movesToGo;
};


struct SearchWorkerShared {
    SearchWorkerShared(
        Depth maxDepth,
        Depth maxDepthMate,
        bool  isInfinite,
        ThreadPool& threadPool,
        NNUE::Networks& NNUEs,
        TranspositionTable& tt
    ) :
        maxDepth(maxDepth),
        maxDepthMate(maxDepthMate),
        isInfinite(isInfinite),
        threads(threadPool),
        networks(NNUEs),
        tt(tt)
    {}

    const Depth             maxDepth;
    const Depth             maxDepthMate;
    const bool              isInfinite;
    ThreadPool&             threads;
    const NNUE::Networks&   networks;
    TranspositionTable&     tt;
};


struct RootMove {
    RootMove() {}
    RootMove(Move m) {
        pv.push_back(m);
    }
    Value score = -VALUE_INFINITE;
    Depth selDepth = 0;
    MoveList pv;
};

using RootMoveList = ValueList<RootMove, MAX_MOVE>;


enum NodeType {
    PV,
    NON_PV,
    ROOT
};


class SearchWorker {
public:
    SearchWorker(SearchWorkerShared& sharedState, size_t idx) :
        idx(idx),
        maxDepth(sharedState.maxDepth),
        maxDepthMate(sharedState.maxDepthMate),
        isInfinite(sharedState.isInfinite),
        threads(sharedState.threads),
        tt(sharedState.tt),
        networks(sharedState.networks),
        cacheTable(networks)
    {
        clear();
    }

    void clear();

    void onNewPv(
        SearchWorker bestWorker,
        const ThreadPool& threads,
        const TranspositionTable tt,
        Depth depth
    );

    void startSearch();
    inline bool isFirstThread() const { return idx == 0; }
    inline RootMove getRootMove(const int i) const { return rootMoves[i]; }

    inline uint64_t getNodes()  const { return nodes.load(std::memory_order_relaxed);  }
    inline uint64_t getTbHits() const { return tbHits.load(std::memory_order_relaxed); }

private:
    friend class ThreadPool;

    inline void iterativeDeepening() {
        rootPosition.getSideToMove() == WHITE ? iterativeDeepening<WHITE>() : iterativeDeepening<BLACK>();
    }
    template<Color Me> void iterativeDeepening();

    template<Color Me, NodeType Nt> Value pvSearch();
    template<Color Me, NodeType Nt> Value qSearch();

    size_t   idx;
    Position rootPosition;
    Depth    currentDepth, searchDepth, completedDepth, selDepth;
    const Depth maxDepth, maxDepthMate;
    const bool isInfinite;
    RootMoveList rootMoves;

    ThreadPool&             threads;
    TranspositionTable&     tt;
    const NNUE::Networks&   networks;
    NNUE::AccumulatorCaches cacheTable;

    Value bestPrevScore;

    std::atomic<uint64_t> nodes, tbHits;

};


} // namespace Search


} // namespace Atom

#endif // SEARCH_H
