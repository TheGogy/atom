#include <cassert>
#include <cstdio>
#include <limits>

#include "bitboard.h"
#include "movegen.h"
#include "movepicker.h"
#include "position.h"
#include "tunables.h"
#include "types.h"

namespace Atom {


namespace Movepicker {


// Score moves to pick from them. Inspired by stockfish.
template<Color Me>
template<Movegen::MoveGenType MgType>
void MovePicker<Me>::score() {
    constexpr int MAX_MOVEPICK_VAL = 1 << 20;

    constexpr Color Opp = ~Me;
    Bitboard enemyPawnThreats, enemyMinorThreats = 0ull, enemyRookThreats = 0ull;
    Bitboard allThreatenedPieces, enemies;
    const Bitboard occ = pos.getPiecesBB();

    if constexpr (MgType == Movegen::MG_TYPE_QUIET) {
        // Pawns
        enemyPawnThreats  = allPawnAttacks<Opp>(pos.getPiecesBB(Opp, PAWN));

        // Knights
        enemies = pos.getPiecesBB(Opp, KNIGHT);
        bitloop(enemies) {
            enemyMinorThreats |= attacks<KNIGHT>(bitscan(enemies), occ);
        }

        // Bishops
        enemies = pos.getPiecesBB(Opp, BISHOP);
        bitloop(enemies) {
            enemyMinorThreats |= attacks<BISHOP>(bitscan(enemies), occ);
        }

        // Rooks
        enemies = pos.getPiecesBB(Opp, ROOK);
        bitloop(enemies) {
            enemyRookThreats |= attacks<ROOK>(bitscan(enemies), occ);
        }

        allThreatenedPieces = (pos.getPiecesBB(Me, KNIGHT, BISHOP) & enemyPawnThreats)
                            | (pos.getPiecesBB(Me, ROOK)  & enemyMinorThreats)
                            | (pos.getPiecesBB(Me, QUEEN) & enemyRookThreats);
    }

    for (ScoredMove& sm : movelist) {
        // Quiet moves
        if constexpr (MgType == Movegen::MG_TYPE_QUIET) {
            const Square from  = moveFrom(sm.move);
            const Square to    = moveTo(sm.move);
            const PieceType pt = typeOf(pos.getPieceAt(from));

            // Killer move bonus
            sm.score =  (sm.move == killer) * Tunables::MOVEPICK_KILLER_SCORE;

            // Check bonus
        sm.score += (pos.givesCheck<Me>(sm.move)) * Tunables::MOVEPICK_CHECK_SCORE;

            // Escape from threat bonus
            sm.score += allThreatenedPieces & from ? (
                pt == QUEEN    && !(to & enemyRookThreats)  ? Tunables::MOVEPICK_ESCAPE_QUEEN
              : pt == ROOK     && !(to & enemyMinorThreats) ? Tunables::MOVEPICK_ESCAPE_ROOK
              : /*pawn or minor*/ !(to & enemyPawnThreats)  ? Tunables::MOVEPICK_ESCAPE_MINOR
              : 0
            ) : 0;

            // Shouldn't put piece in harm's way (en prise if you want to be fancy)
            sm.score -= (
                pt == QUEEN ? bool(to & enemyRookThreats)  * Tunables::MOVEPICKER_ENPRISE_QUEEN
              : pt == ROOK  ? bool(to & enemyMinorThreats) * Tunables::MOVEPICKER_ENPRISE_ROOK
              :               bool(to & enemyPawnThreats)  * Tunables::MOVEPICKER_ENPRISE_MINOR
            );
        }

        // Tactical moves
        else if constexpr (MgType == Movegen::MG_TYPE_TACTICAL) {
            sm.score = Tunables::MOVEPICK_CAPTURE_MULTIPLIER * PIECE_VALUE[pos.getPieceAt(moveTo(sm.move))];
        }

        // Evasions
        else {
            const Square to = moveTo(sm.move);
            const Piece cap = pos.getPieceAt(to);

            if (cap || (moveTypeOf(sm.move) == MT_PROMOTION && movePromotionType(sm.move) == QUEEN)) {
                sm.score = PIECE_VALUE[cap] + MAX_MOVEPICK_VAL - typeOf(pos.getPieceAt(moveFrom(sm.move)));
            } else {
                sm.score = 0;
            }
        }
    }
}


template void MovePicker<WHITE>::score<Movegen::MG_TYPE_ALL>();
template void MovePicker<WHITE>::score<Movegen::MG_TYPE_TACTICAL>();
template void MovePicker<WHITE>::score<Movegen::MG_TYPE_QUIET>();
template void MovePicker<WHITE>::score<Movegen::MG_TYPE_EVASIONS>();
template void MovePicker<BLACK>::score<Movegen::MG_TYPE_ALL>();
template void MovePicker<BLACK>::score<Movegen::MG_TYPE_TACTICAL>();
template void MovePicker<BLACK>::score<Movegen::MG_TYPE_QUIET>();
template void MovePicker<BLACK>::score<Movegen::MG_TYPE_EVASIONS>();


template<Color Me>
template<MovePickType MpType, typename Pred>
ScoredMove MovePicker<Me>::select(Pred filter) {
    while (current < endMoves) {
        if constexpr (MpType == MP_TYPE_BEST) {
            std::swap(*current, *std::max_element(current, endMoves));
        }

        if (current->move != ttMove && filter()) {
            return *current++;
        }

        current++;
    }

    return ScoredMove(MOVE_NONE, 0);
}



template<Color Me>
Move MovePicker<Me>::nextMove(bool skipQuiet) {
top:
    switch (mpStage) {

        // For transposition stages, just return the move in the transposition table
        case MovePickStage::MP_STAGE_TT:
        case MovePickStage::MP_STAGE_EVASION_TT:
        case MovePickStage::MP_STAGE_QSEARCH_ALL_TT:
            ++mpStage;
            return ttMove;

        // Generate all moves for stage
        case MovePickStage::MP_STAGE_CAPTURE_GENERATE:
        case MovePickStage::MP_STAGE_QSEARCH_CAP_GENERATE:
            current  = endBadCaptures = movelist;
            endMoves = Movegen::enumerateLegalMovesToList<Me, Movegen::MG_TYPE_TACTICAL>(pos, current);

            MovePicker<Me>::score<Movegen::MG_TYPE_TACTICAL>();
            kSort(current, endMoves, std::numeric_limits<int>::min());
            ++mpStage;

            // Go back to the beginning to process stage
            goto top;

        // Process good capture moves
        case MovePickStage::MP_STAGE_CAPTURE_GOOD:

            // Return next move if it isn't in the TT
            if (MovePicker<Me>::select<MovePickType::MP_TYPE_NEXT>([&]() {
                // See if we win the current capture if we make all trades
                // If we don't, move the capture to endBadCaptures
                return pos.see(current->move, -current->score / Tunables::MOVEPICKER_LOSING_CAP_THRESHOLD)
                        ? true : (*endBadCaptures++ = *current, false);
            }))
            {
                return (current - 1)->move;
            }

            ++mpStage;
            [[fallthrough]];

        // Generate the quiet moves
        case MovePickStage::MP_STAGE_QUIET_GENERATE:
            if (!skipQuiet) {
                current = endBadCaptures;
                endMoves = beginBadQuiets = endBadQuiets = Movegen::enumerateLegalMovesToList<Me, Movegen::MG_TYPE_QUIET>(pos, current);

                MovePicker<Me>::score<Movegen::MG_TYPE_QUIET>();
                kSort(current, endMoves, depth * Tunables::MOVEPICKER_QUIET_THRESHOLD);
            }
            ++mpStage;
            [[fallthrough]];

        // Find good quiet moves
        case MovePickStage::MP_STAGE_QUIET_GOOD:
            // Return next move if it isn't in the TT
            if (!skipQuiet && MovePicker<Me>::select<MP_TYPE_NEXT>([]() { return true; })) {

                // Check to see if we still have good quiet moves
                if (
                    (current - 1)->score > Tunables::MOVEPICKER_GOOD_QUIET_THRESHOLD
                 || (current - 1)->score <= (depth * Tunables::MOVEPICKER_QUIET_THRESHOLD)
                ) {
                    return (current - 1)->move;
                }

                // We don't have any more good quiets: move to the bad ones
                beginBadQuiets = current - 1;
            }

        current = movelist;
        endMoves = endBadCaptures;
        ++mpStage;
        [[fallthrough]];

        // Find bad capture moves
        case MovePickStage::MP_STAGE_CAPTURE_BAD:
            // Return next move if it isn't in the TT
            if (MovePicker<Me>::select<MP_TYPE_NEXT>([]() { return true; })) {
                return (current - 1)->move;
            }

            current = beginBadQuiets;
            endMoves = endBadQuiets;

            ++mpStage;
            [[fallthrough]];

        // Find bad quiet moves
        case MovePickStage::MP_STAGE_QUIET_BAD:
            if (!skipQuiet) {
                // Return next move if it isn't in the TT
                return MovePicker<Me>::select<MP_TYPE_NEXT>([]() { return true; }).move;
            }

            // If we aren't in evasions or qsearch, the search ends here:
            // return no move
            return MOVE_NONE;

        // Generate evasion moves
        case MovePickStage::MP_STAGE_EVASION_GENERATE:
            current = movelist;
            endMoves = Movegen::enumerateLegalMovesToList<Me, Movegen::MG_TYPE_EVASIONS>(pos, current);

            MovePicker<Me>::score<Movegen::MG_TYPE_EVASIONS>();
            ++mpStage;
            [[fallthrough]];

        case MovePickStage::MP_STAGE_EVASION_GOOD:
            // Return next move if it isn't in the TT (ordered)
            return MovePicker<Me>::select<MP_TYPE_BEST>([]() { return true; }).move;

        case MovePickStage::MP_STAGE_QSEARCH_CAP_GOOD:
            // Return next move if it isn't in the TT
            if (MovePicker<Me>::select<MP_TYPE_NEXT>([]() { return true; })) {
                return (current - 1)->move;
            }

            if (depth <= QSEARCH_DEPTH_NORMAL) return MOVE_NONE;

            ++mpStage;
            [[fallthrough]];

        case MovePickStage::MP_STAGE_QSEARCH_CHK_GENERATE:
            current = movelist;
            endMoves = Movegen::enumerateChecksToList<Me, Movegen::MG_TYPE_QUIET>(pos, current);

            ++mpStage;
            [[fallthrough]];

        case MovePickStage::MP_STAGE_QSEARCH_CHK_GOOD:
            // Return next move if it isn't in the TT
            return MovePicker<Me>::select<MP_TYPE_NEXT>([]() { return true; }).move;

    }

    // Should never reach this point.
    assert(false);
    return MOVE_NONE;
}

template Move MovePicker<WHITE>::nextMove(bool skipQuiet);
template Move MovePicker<BLACK>::nextMove(bool skipQuiet);


} // namespace Movepicker

} // namespace Atom
