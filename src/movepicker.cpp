
#include "bitboard.h"
#include "movegen.h"
#include "movepicker.h"
#include "position.h"
#include "tunables.h"
#include "types.h"


namespace Atom {

namespace Movepicker {


template<Color Me>
MovePicker::MovePicker(const Position& pos, Move ttMove, Move killer, Depth depth)
    : pos(pos), ttMove(ttMove), killer(killer), depth(depth),
      stage(determineStage<Me>(pos.inCheck(), ttMove, depth))
{}

// Score moves to pick from them. Inspired by stockfish.
template<Color Me, Movegen::MoveGenType MgType>
void MovePicker::score() {
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
            sm.score += (pos.wouldGiveCheck<Me>(pt, to)) * Tunables::MOVEPICK_CHECK_SCORE;

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


} // namespace Movepicker

} // namespace Atom
