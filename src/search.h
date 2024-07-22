#ifndef SEARCH_H
#define SEARCH_H

#include <chrono>
#include <cstdint>
#include <vector>

#include "types.h"

namespace Atom {

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

} // namespace Atom

#endif // SEARCH_H
