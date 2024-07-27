#ifndef MOVEPICKER_H
#define MOVEPICKER_H

#include <cstdint>

#include "position.h"
#include "movegen.h"

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

    MP_STAGE_END
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


inline void kSort(Move* start, Move* end, int limit) {
    for (Move* sortedEnd = start, *current = start + 1; current < end; ++current) {
        if (*current >= limit) {
            Move temp = *current;
            *current = *++sortedEnd;
            Move* insertionPoint = sortedEnd;
            while (insertionPoint != start && *(insertionPoint - 1) < temp) {
                *insertionPoint = *(insertionPoint - 1);
                --insertionPoint;
            }
            *insertionPoint = temp;
        }
    }
}


enum MovePickType {
    MP_TYPE_MAIN,
    MP_TYPE_BEST
};


struct ScoredMove {
    inline ScoredMove() {}
    inline ScoredMove(Move move, Value score) : move(move), score(score) {}
    Move  move;
    Value score;
};

inline bool operator<(const ScoredMove& a, const ScoredMove& b) { return a.score < b.score; }


using ScoredMoveList = ValueList<ScoredMove, MAX_MOVE>;

class MovePicker {
public:
    template<Color Me>
    MovePicker(
        const Position& pos,
        Move  ttMove,
        Move  killer,
        Depth depth
    );

    // MovePicker cannot be copied
    MovePicker(const MovePicker &)            = delete;
    MovePicker(MovePicker &&)                 = delete;
    MovePicker &operator=(const MovePicker &) = delete;
    MovePicker &operator=(MovePicker &&)      = delete;

    Move next();

private:
    const Position& pos;
    Move            ttMove, killer;
    Depth           depth;
    MovePickStage   stage;
    ScoredMoveList  movelist;

    template<Color Me>
    inline MovePickStage determineStage(const bool inCheck, Move ttMove, Depth depth) const {
        if (pos.inCheck()) {
            return MovePickStage::MP_STAGE_EVASION_TT + !(ttMove && pos.isPseudoLegalMove<Me>(ttMove));
        } else {
            return (depth > 0 ? MovePickStage::MP_STAGE_TT : MovePickStage::MP_STAGE_QSEARCH_ALL_TT) + !(ttMove && pos.isPseudoLegalMove<Me>(ttMove));
        }
    }

    template<Color Me, Movegen::MoveGenType MgType>
    void score();
};



} // namespace Movepicker

} // namespace Atom

#endif // MOVEPICKER_H
