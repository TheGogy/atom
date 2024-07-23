#ifndef SEARCH_H
#define SEARCH_H

#include <chrono>
#include <cstdint>
#include <vector>

#include "nnue/network.h"
#include "types.h"

namespace Atom {

// Declare these here: they are defined in thread.h
class ThreadPool;

namespace Search {

using TimePoint = std::chrono::milliseconds::rep;

inline TimePoint now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

struct SearchLimits {
    std::vector<std::string> searchMoves;
    TimePoint time[COLOR_NB], inc[COLOR_NB];
    TimePoint startTimePoint, moveTime;
    bool canPonder, isInfinite;
    uint64_t nodes;
    int depth, mate, movesToGo;
};

struct SearchWorkerShared {
    SearchWorkerShared(
        ThreadPool& threadPool,
        NNUE::Networks& NNUEs
    ) :
    threads(threadPool),
    networks(NNUEs) {}

    ThreadPool& threads;
    const NNUE::Networks& networks;
};

class SearchWorker {
public:

    void startSearch();

    void clear();
};

} // namespace Search

} // namespace Atom

#endif // SEARCH_H
