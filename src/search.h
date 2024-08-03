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
#include "tunables.h"
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

// Some small functions to calculate values used in search

inline Value clampEval(Value eval) {
    return std::clamp(eval, VALUE_TB_LOSS_IN_MAX_PLY + 1, VALUE_TB_WIN_IN_MAX_PLY - 1);
}


inline Value futilityMargin(Depth depth, bool ttCut, bool improving, bool oppWorsening, int statScore) {
    Value futility     = Tunables::FUTILITY_MULT_BASE + ttCut * Tunables::FUTILITY_TTCUT_IMPACT;
    Value improvement  = improving    * futility * Tunables::FUTILITY_IMPROVEMENT_SCALE;
    Value worsening    = oppWorsening * futility / Tunables::FUTILITY_WORSENING_SCALE;
    Value statScoreAdj = statScore / Tunables::FUTILITY_STAT_SCALE;

    return (futility * depth) - improvement - worsening - statScoreAdj;
}


inline Depth getNullMoveReductionAmount(Value eval, int beta, Depth depth) {
    return std::min((eval - beta) / Tunables::NMR_EVAL_SCALE, Tunables::NMR_EVAL_MAX_DIFF)
           + (depth / Tunables::NMR_DEPTH_SCALE) + Tunables::NMR_MIN_REDUCTION;
}



inline void updatePv(Move* pv, Move currentMove, const Move* childPv) {
    for (*pv++ = currentMove; childPv && *childPv != MOVE_NONE; ) {
        *pv++ = *childPv++;
    }
    *pv = MOVE_NONE;
}


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
    SearchLimits() {
        time[WHITE] = time[BLACK] = TimePoint(0);
        inc[WHITE]  = inc[BLACK]  = TimePoint(0);
        isInfinite = false;
        nodes = depth = mate = movesToGo = 0;
    }

    std::vector<std::string> searchMoves;
    TimePoint time[COLOR_NB], inc[COLOR_NB];
    TimePoint startTimePoint, moveTime;
    bool isInfinite;
    uint64_t nodes;
    int depth, mate, movesToGo;
};


struct SearchWorkerShared {
    SearchWorkerShared(
        ThreadPool& threadPool,
        NNUE::Networks& NNUEs,
        TranspositionTable& tt
    ) :
        threads(threadPool),
        networks(NNUEs),
        tt(tt)
    {}

    ThreadPool&             threads;
    const NNUE::Networks&   networks;
    TranspositionTable&     tt;
};


struct RootMove {
    RootMove() {}
    RootMove(Move m) {
        pv.push_back(m);
    }

    // Used for sorting

    bool operator== (const Move& m) const { return pv[0] == m; }
    bool operator== (const RootMove& rm) const { return pv[0] == rm.pv[0]; }
    bool operator<  (const RootMove& rm) const {
        return rm.score == score ? rm.prevScore < prevScore : rm.score < score;
    }

    Value score     = -VALUE_INFINITE;
    Value prevScore = -VALUE_INFINITE;
    Value avgScore  = -VALUE_INFINITE;
    Value uciScore  = -VALUE_INFINITE;
    Depth selDepth  = 0;
    MoveList pv;
};

using RootMoveList = ValueList<RootMove, MAX_MOVE>;


enum NodeType {
    NODETYPE_PV,
    NODETYPE_NON_PV,
    NODETYPE_ROOT
};


struct StackObject {
    Move*   pv;
    int     ply;
    Value   staticEval;
    Move    currentMove;
    Move    killer;
    bool    inCheck, ttHit, ttPv;
    int     statScore;
    int     moveCount;
};


class SearchWorker {
public:
    void clear();

    SearchWorker(SearchWorkerShared& sharedState, size_t idx) :
        idx(idx),
        threads(sharedState.threads),
        tt(sharedState.tt),
        networks(sharedState.networks),
        cacheTable(networks)
    {
        clear();
    }

    void onNewPv(
        SearchWorker& bestWorker,
        const ThreadPool& threads,
        const TranspositionTable& tt,
        Depth depth
    );

    void startSearch();
    inline bool isFirstThread() const { return idx == 0; }
    inline RootMove getRootMove(const int i) const { return rootMoves[i]; }

    inline uint64_t getNodes()  const { return nodes.load(std::memory_order_relaxed);  }
    inline uint64_t getTbHits() const { return tbHits.load(std::memory_order_relaxed); }

    Search::SearchLimits limits;
    Position rootPosition;
    RootMoveList rootMoves;

private:
    friend class ThreadPool;


    // Searching functions
    inline void iterativeDeepening() {
        rootPosition.getSideToMove() == WHITE ? iterativeDeepening<WHITE>() : iterativeDeepening<BLACK>();
    }
    template<Color Me> void iterativeDeepening();


    template<Color Me, NodeType Nt>
    Value pvSearch(
        Position& pos, StackObject* sPtr, Value alpha, Value beta, Depth depth, bool cutNode
    );


    template<Color Me, NodeType Nt> 
    Value qSearch(
        Position& pos, StackObject* sPtr, Value alpha, Value beta, Depth depth
    );


    inline int getReduction(bool improving, Depth depth, int moveCount, int delta) {
        int scale = reductions[depth] * reductions[moveCount];
        return (
            (scale + Tunables::REDUCTION_BASE - delta * Tunables::REDUCTION_DELTA_SCALE / rootDelta) / Tunables::REDUCTION_NORMALISER
          + (!improving && scale > Tunables::REDUCTION_SCALE_THRESHOLD)
        );
    }


    size_t   idx;

    Depth    currentDepth, searchDepth, completedDepth, selDepth, nmpCutoff;
    Value    rootDelta;

    std::array<int, MAX_MOVE> reductions;

    ThreadPool&             threads;
    TranspositionTable&     tt;
    const NNUE::Networks&   networks;
    NNUE::AccumulatorCaches cacheTable;

    std::atomic<uint64_t> nodes, tbHits;

};


} // namespace Search


} // namespace Atom

#endif // SEARCH_H
