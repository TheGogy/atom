#ifndef MOVEPICKER_H
#define MOVEPICKER_H

#include <cstdint>

#include "position.h"
#include "movegen.h"
#include "types.h"

namespace Atom {

namespace Movepicker {


enum class MovePickStage : uint32_t {
    MP_STAGE_TT = 0,

    MP_STAGE_CAPTURE_GENERATE,
    MP_STAGE_CAPTURE_GOOD,

    MP_STAGE_QUIET_GENERATE,
    MP_STAGE_QUIET_GOOD,

    MP_STAGE_CAPTURE_BAD,
    MP_STAGE_QUIET_BAD,

    MP_STAGE_EVASION_TT,
    MP_STAGE_EVASION_GENERATE,
    MP_STAGE_EVASION_GOOD,

    MP_STAGE_QSEARCH_ALL_TT,
    MP_STAGE_QSEARCH_CAP_GENERATE,
    MP_STAGE_QSEARCH_CAP_GOOD,
    MP_STAGE_QSEARCH_CHK_GENERATE,
    MP_STAGE_QSEARCH_CHK_GOOD,
};


inline MovePickStage operator++(MovePickStage &s) {
    s = static_cast<MovePickStage>(static_cast<uint32_t>(s) + 1);
    return s;
}

inline bool operator< (MovePickStage a, MovePickStage b) {
    return static_cast<uint32_t>(a) < static_cast<uint32_t>(b);
}

inline MovePickStage operator+(MovePickStage s, int i) {
    return static_cast<MovePickStage>(static_cast<uint32_t>(s) + i);
}


inline void kSort(ScoredMove* start, ScoredMove* end, int limit) {
    for (ScoredMove* sortedEnd = start, *current = start + 1; current < end; ++current) {
        if (current->score >= limit) {
            ScoredMove temp = *current;
            *current = *++sortedEnd;
            ScoredMove* insertionPoint = sortedEnd;
            while (insertionPoint != start && *(insertionPoint - 1) < temp) {
                *insertionPoint = *(insertionPoint - 1);
                --insertionPoint;
            }
            *insertionPoint = temp;
        }
    }
}


enum MovePickType {
    MP_TYPE_NEXT,
    MP_TYPE_BEST
};

template<Color Me>
class MovePicker {
public:
    MovePicker(
        const Position& pos,
        Move  ttMove,
        Move  killer,
        Depth depth
    ) : pos(pos), ttMove(ttMove), killer(killer), depth(depth)
    {
        mpStage = determineStage(pos.inCheck(), ttMove, depth);
    }

    // MovePicker cannot be copied
    MovePicker(const MovePicker &)            = delete;
    MovePicker(MovePicker &&)                 = delete;
    MovePicker &operator=(const MovePicker &) = delete;
    MovePicker &operator=(MovePicker &&)      = delete;

    Move nextMove(bool skipQuiet = false);

private:
    const Position& pos;
    Move            ttMove, killer;
    Depth           depth;
    MovePickStage   mpStage;
    ScoredMove      movelist[MAX_MOVE];
    ScoredMove      *current, *endMoves, *endBadCaptures, *beginBadQuiets, *endBadQuiets;

    inline MovePickStage determineStage(const bool inCheck, Move ttMove, Depth depth) const {
        if (pos.inCheck()) {
            return MovePickStage::MP_STAGE_EVASION_TT + !(ttMove && pos.isPseudoLegalMove<Me>(ttMove));
        } else {
            return (depth > 0 ? MovePickStage::MP_STAGE_TT : MovePickStage::MP_STAGE_QSEARCH_ALL_TT) + !(ttMove && pos.isPseudoLegalMove<Me>(ttMove));
        }
    }

    template<Movegen::MoveGenType MgType>
    void score();

    template<MovePickType MpType, typename Pred>
    ScoredMove select(Pred filter);
};



} // namespace Movepicker

} // namespace Atom

#endif // MOVEPICKER_H
