#pragma once

#include <cstdint>

#include "position.h"
#include "movegen.h"
#include "types.h"

namespace Atom {

namespace Movepicker {

// Move history
template<typename T, int D>
class StatsEntry {

    T entry;

public:
    void operator=(const T& v) { entry = v; }
    T*   operator&() { return &entry; }
    T*   operator->() { return &entry; }
    operator const T&() const { return entry; }

    void operator<<(int bonus) {
        static_assert(D <= std::numeric_limits<T>::max(), "D overflows T");

        // Make sure that bonus is in range [-D, D]
        int clampedBonus = std::clamp(bonus, -D, D);
        entry += clampedBonus - entry * std::abs(clampedBonus) / D;

        assert(std::abs(entry) <= D);
    }
};

template<typename T, int D, int Size, int... Sizes>
struct Stats: public std::array<Stats<T, D, Sizes...>, Size> {
    using stats = Stats<T, D, Size, Sizes...>;

    void fill(const T& v) {

        // For standard-layout 'this' points to the first struct member
        assert(std::is_standard_layout_v<stats>);

        using entry = StatsEntry<T, D>;
        entry* p    = reinterpret_cast<entry*>(this);
        std::fill(p, p + sizeof(*this) / sizeof(entry), v);
    }
};

template<typename T, int D, int Size>
struct Stats<T, D, Size>: public std::array<StatsEntry<T, D>, Size> {};


// Pawn and correction history.
constexpr int PAWN_HISTORY_SIZE        = 512;
constexpr int CORRECTION_HISTORY_SIZE  = 16384;
constexpr int CORRECTION_HISTORY_LIMIT = 1024;

enum PawnHistoryType {
    PH_NORMAL,
    PH_CORRECTION
};

template<PawnHistoryType T = PH_NORMAL>
inline int pawnStructureIndex(const Position& pos) {
    return pos.pawnKey() & ((T == PH_NORMAL ? PAWN_HISTORY_SIZE : CORRECTION_HISTORY_SIZE) - 1);
}

using ButterflyHistory      = Stats<int16_t, 7183, COLOR_NB, int(SQUARE_NB) * int(SQUARE_NB)>;
using CapturePieceToHistory = Stats<int16_t, 10692, PIECE_NB, SQUARE_NB, PIECE_TYPE_NB>;
using PieceToHistory        = Stats<int16_t, 29952, PIECE_NB, SQUARE_NB>;
using ContinuationHistory   = Stats<PieceToHistory, 0, PIECE_NB, SQUARE_NB>;
using PawnHistory           = Stats<int16_t, 8192, PAWN_HISTORY_SIZE, PIECE_NB, SQUARE_NB>;
using CorrectionHistory     = Stats<int16_t, CORRECTION_HISTORY_LIMIT, COLOR_NB, CORRECTION_HISTORY_SIZE>;

enum StatsType {
    NoCaptures,
    Captures
};

// TODO: History struct

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
    for (ScoredMove* sortedEnd = start, *p = start + 1; p < end; ++p)
        if (p->score >= limit)
        {
            ScoredMove tmp = *p, *q;
            *p = *++sortedEnd;
            for (q = sortedEnd; q != start && *(q - 1) < tmp; --q) {
                *q = *(q - 1);
            }
            *q = tmp;
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
        Depth depth,
        const ButterflyHistory* bh,
        const CapturePieceToHistory* cph,
        const PieceToHistory** ch,
        const PawnHistory* ph
    ) :
        pos(pos), ttMove(ttMove), killer(killer), depth(depth),
        butterflyHist(bh), captureHist(cph), continuationHist(ch), pawnHist(ph)
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

    // Histories
    const ButterflyHistory*      butterflyHist;
    const CapturePieceToHistory* captureHist;
    const PieceToHistory**       continuationHist;
    const PawnHistory*           pawnHist;

    inline MovePickStage determineStage(const bool inCheck, Move ttMove, Depth depth) const {
        if (pos.inCheck()) {
            return MovePickStage::MP_STAGE_EVASION_TT + !(ttMove && pos.isPseudoLegalMove<Me>(ttMove));
        } else {
            return (depth > 0 ? MovePickStage::MP_STAGE_TT : MovePickStage::MP_STAGE_QSEARCH_ALL_TT) + !(ttMove && pos.isPseudoLegalMove<Me>(ttMove));
        }
    }

    template<Movegen::MoveGenType MgType>
    void score();

    inline int16_t getCaptureHist(const Move m) {
        return (*captureHist)[pos.getPieceAt(moveFrom(m))][moveTo(m)][typeOf(pos.getPieceAt(moveTo(m)))];
    }

    template<typename Pred>
    ScoredMove select(Pred filter);


    // Allows for iteration over the moves
    ScoredMove* begin() { return current; }
    ScoredMove* end()   { return endMoves; }
};



} // namespace Movepicker

} // namespace Atom
