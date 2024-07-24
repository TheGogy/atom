#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "types.h"
#include "position.h"

namespace Atom {


using MoveList = ValueList<Move, MAX_MOVE>;


namespace Movegen {


#define ENUMERATE_MOVES(...) if (!__VA_ARGS__) return false
#define HANDLE_MOVE(...)     if (!handler(__VA_ARGS__)) return false


// Differentiate between different types of move. This allows
// us to generate only specific move types.
enum MoveGenType {
    QUIET_MOVES = 1,
    TACTICAL_MOVES = 2,
    ALL_MOVES = QUIET_MOVES | TACTICAL_MOVES,
};


// Enumerate a single promotion move
template<Color Me, PieceType PromotionType, typename Handler>
inline bool enumeratePromotion(Square from, Square to, const Handler& handler) {
    HANDLE_MOVE(makeMove<PROMOTION>(from, to, PromotionType));
    return true;
}


// Enumerate all promotion moves for a single pawn.
// This is for all piece types.
// Queens are regarded as tactical;
// Rooks, Knights and Bishops are regarded as quiet.
template<Color Me, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumeratePromotions(Square from, Square to, const Handler& handler) {
    if constexpr (MGType & TACTICAL_MOVES) {
        ENUMERATE_MOVES(enumeratePromotion<Me, QUEEN>(from, to, handler));
    }

    if constexpr (MGType & QUIET_MOVES) {
        ENUMERATE_MOVES(enumeratePromotion<Me, KNIGHT>(from, to, handler));
        ENUMERATE_MOVES(enumeratePromotion<Me, ROOK>(from, to, handler));
        ENUMERATE_MOVES(enumeratePromotion<Me, BISHOP>(from, to, handler));
    }

    return true;
}


// Enumerates all pawn promotion moves, including checks.
// This is for all pawns on the promotion rank.
template<Color Me, bool InCheck, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumeratePawnPromotionMoves(const Position &pos, Bitboard source, const Handler& handler) {
    constexpr Color Opp = ~Me;

    constexpr Bitboard Rank7    = (Me == WHITE) ? RANK_7_BB : RANK_2_BB;
    constexpr Direction Up      = (Me == WHITE) ? NORTH : SOUTH;

    constexpr Direction UpLeft  = (Me == WHITE) ? NORTH_WEST : SOUTH_EAST;
    constexpr Direction UpRight = (Me == WHITE) ? NORTH_EAST : SOUTH_WEST;

    const Bitboard emptyBB   = pos.getEmptyBB();
    const Bitboard pinOrtho  = pos.pinOrtho();
    const Bitboard pinDiag   = pos.pinDiag();
    const Bitboard checkMask = pos.checkMask();

    // Promoting pawns are pawns that are on the 7th rank.
    // Orthogonally pinned pawns cannot promote.
    // https://lichess.org/editor/3K4/3P4/8/8/3r1k2/8/8/8_w_-_-_0_1?color=white
    // https://lichess.org/editor/3rn3/3P1k2/8/8/3K4/8/8/8_w_-_-_0_1?color=white
    const Bitboard pawnsCanPromote = source & Rank7 & ~pinOrtho;

    if (pawnsCanPromote) {
        // Capture Promotion
        {
            // Pawns can capture left and right only if the piece they are
            // capturing is in the pinmask, or if the pawn is not pinned.
            // https://lichess.org/editor/2b1b3/3P2k1/8/1K6/8/8/8/8_w_-_-_0_1?color=white
            Bitboard capLPromotions = (shift<UpLeft>(pawnsCanPromote & ~pinDiag)  | (shift<UpLeft>(pawnsCanPromote & pinDiag) & pinDiag)) & pos.getPiecesBB(Opp);
            Bitboard capRPromotions = (shift<UpRight>(pawnsCanPromote & ~pinDiag) | (shift<UpRight>(pawnsCanPromote & pinDiag) & pinDiag)) & pos.getPiecesBB(Opp);

            if constexpr (InCheck) {
                capLPromotions &= checkMask;
                capRPromotions &= checkMask;
            }

            bitloop(capLPromotions) {
                Square to = bitscan(capLPromotions);
                Square from = to - UpLeft;
                ENUMERATE_MOVES(enumeratePromotions<Me, MGType>(from, to, handler));
            }
            bitloop(capRPromotions) {
                Square to = bitscan(capRPromotions);
                Square from = to - UpRight;
                ENUMERATE_MOVES(enumeratePromotions<Me, MGType>(from, to, handler));
            }
        }

        // Quiet Promotion
        {
            // Pawns can capture forward as long as the square in front of them is empty
            // and they are not pinned. We have already filtered out orthogonal pins:
            // we only need to filter out diagonal pins.
            Bitboard quietPromotions = shift<Up>(pawnsCanPromote & ~pinDiag) & emptyBB;

            if constexpr (InCheck) quietPromotions &= checkMask;

            bitloop(quietPromotions) {
                Square to = bitscan(quietPromotions);
                Square from = to - Up;
                ENUMERATE_MOVES(enumeratePromotions<Me, MGType>(from, to, handler));
            }
        }
    }

    return true;
}


// Enumerate pawn en passant moves.
template<Color Me, bool InCheck, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumeratePawnEnpassantMoves(const Position &pos, Bitboard source, const Handler& handler) {
    constexpr Color Opp = ~Me;
    constexpr Direction pawnDir = pawnDirection(Me);
    constexpr Bitboard epRank = (Me == WHITE ? RANK_5_BB : RANK_4_BB);

    const Bitboard pinOrtho  = pos.pinOrtho();
    const Bitboard pinDiag   = pos.pinDiag();
    const Bitboard checkMask = pos.checkMask();

    if (pos.getEpSquare() != SQ_NONE) {

        // The piece that we will capture through en passant
        const Bitboard epcaptured = sqToBB(pos.getEpSquare() - pawnDir);

        // Our pieces that can take through en passant. Orthogonally pinned pawns cannot take.
        Bitboard enpassants = pawnAttacks(Opp, pos.getEpSquare()) & source & ~pinOrtho;

        if constexpr (InCheck) {
            // If we are in check, we should only add the checkmask if it is not the e.p.
            // piece putting us in check, as the en passant piece is not in the checkmask.
            if (!(pos.checkers() & epcaptured)) {
                enpassants &= checkMask;
            }
        }

        bitloop(enpassants) {
            const Square from = bitscan(enpassants);

            // If our pawn is diagonally pinned and the en passant square is not part of the pinmask,
            // then we cannot take
            // https://lichess.org/editor/5b2/5k2/8/2Pp4/1K6/8/8/8_w_-_-_0_1?color=white
            // https://lichess.org/editor/5b2/5k2/8/1pP5/1K6/8/8/8_w_-_-_0_1?color=white
            if ((sqToBB(from) & pinDiag) && !(sqToBB(pos.getEpSquare()) & pinDiag))
                continue;

            // If our king is on the same rank as one of the opponent's sliders, and if by removing our pawn and the opponent's pawn,
            // then we would be in check, then we cannot take en passant. Otherwise, add capture.
            // https://lichess.org/editor/8/6k1/8/1K1Pp2q/8/8/8/8_w_-_-_0_1?color=white
            // https://lichess.org/editor/8/6k1/8/1KPPp2q/8/8/8/8_w_-_-_0_1?color=white
            if (!((epRank & pos.getPiecesBB(Me, KING)) && (epRank & pos.getPiecesBB(Opp, ROOK, QUEEN))) ||
                !(attacks<ROOK>(pos.getKingSquare(Me), pos.getPiecesBB() ^ sqToBB(from) ^ epcaptured) & pos.getPiecesBB(Opp, ROOK, QUEEN))) {
                Square to = pos.getEpSquare();
                HANDLE_MOVE(makeMove<EN_PASSANT>(from, to));
            }
        }
    }

    return true;
}


// Generate normal pawn moves.
// This includes normal pushes and captures.
template<Color Me, bool InCheck, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumeratePawnNormalMoves(const Position &pos, Bitboard source, const Handler& handler) {
    constexpr Color Opp = ~Me;
    constexpr Bitboard Rank3 = (Me == WHITE) ? RANK_3_BB : RANK_6_BB;
    constexpr Bitboard Rank7 = (Me == WHITE) ? RANK_7_BB : RANK_2_BB;
    constexpr Direction Up      = (Me == WHITE) ? NORTH : SOUTH;
    constexpr Direction UpLeft  = (Me == WHITE) ? NORTH_WEST : SOUTH_EAST;
    constexpr Direction UpRight = (Me == WHITE) ? NORTH_EAST : SOUTH_WEST;

    Bitboard emptyBB   = pos.getEmptyBB();
    Bitboard pinOrtho  = pos.pinOrtho();
    Bitboard pinDiag   = pos.pinDiag();
    Bitboard checkMask = pos.checkMask();

    // Single & Double Push
    if constexpr (MGType & QUIET_MOVES) {

        // Diagonally pinned pawns can never push, as that would be an orthogonal move.
        Bitboard pawns = source & ~Rank7 & ~pinDiag;
        Bitboard singlePushes = (shift<Up>(pawns & ~pinOrtho) | (shift<Up>(pawns & pinOrtho) & pinOrtho)) & emptyBB;
        Bitboard doublePushes = shift<Up>(singlePushes & Rank3) & emptyBB;

        if constexpr (InCheck) {
            singlePushes &= checkMask;
            doublePushes &= checkMask;
        }

        bitloop(singlePushes) {
            Square to = bitscan(singlePushes);
            Square from = to - Up;
            HANDLE_MOVE(makeMove(from, to));
        }

        bitloop(doublePushes) {
            Square to = bitscan(doublePushes);
            Square from = to - Up - Up;
            HANDLE_MOVE(makeMove(from, to));
        }
    }

    // Normal Capture
    if constexpr (MGType & TACTICAL_MOVES) {
        // Orthogonally pinned pawns cannot take, as that would be a diagonal move.
        Bitboard pawns = source & ~Rank7 & ~pinOrtho;
        Bitboard capL = (shift<UpLeft>(pawns & ~pinDiag)  | (shift<UpLeft>(pawns & pinDiag) & pinDiag)) & pos.getPiecesBB(Opp);
        Bitboard capR = (shift<UpRight>(pawns & ~pinDiag) | (shift<UpRight>(pawns & pinDiag) & pinDiag)) & pos.getPiecesBB(Opp);

        if constexpr (InCheck) {
            capL &= checkMask;
            capR &= checkMask;
        }

        bitloop(capL) {
            Square to = bitscan(capL);
            Square from = to - UpLeft;
            HANDLE_MOVE(makeMove(from, to));
        }
        bitloop(capR) {
            Square to = bitscan(capR);
            Square from = to - UpRight;
            HANDLE_MOVE(makeMove(from, to));
        }
    }

    return true;
}


// Enumerates all pawn moves.
template<Color Me, bool InCheck, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumeratePawnMoves(const Position &pos, Bitboard source, const Handler& handler) {
    ENUMERATE_MOVES(enumeratePawnNormalMoves<Me, InCheck, MGType, Handler>(pos, source, handler));
    ENUMERATE_MOVES(enumeratePawnPromotionMoves<Me, InCheck, MGType, Handler>(pos, source, handler));

    if constexpr (MGType & TACTICAL_MOVES) {
        ENUMERATE_MOVES(enumeratePawnEnpassantMoves<Me, InCheck, MGType, Handler>(pos, source, handler));
    }

    return true;
}


// Enumerates all castling moves.
template<Color Me, typename Handler>
inline bool enumerateCastlingMoves(const Position &pos, const Handler& handler) {
    Square kingSquare = pos.getKingSquare(Me);
    const CastlingRight kingSide  = Me & KING_SIDE;
    const CastlingRight queenSide = Me & QUEEN_SIDE;

    // King side castling
    if (pos.canCastle(kingSide) && pos.isEmpty(CastlingPath[kingSide]) && !(pos.threatened() & CastlingKingPath[kingSide])) {
        Square from = kingSquare;
        Square to = CastlingKingTo[kingSide];
        HANDLE_MOVE(makeMove<CASTLING>(from, to));
    }

    // Queen side castling
    if (pos.canCastle(queenSide) && pos.isEmpty(CastlingPath[queenSide]) && !(pos.threatened() & CastlingKingPath[queenSide])) {
        Square from = kingSquare;
        Square to = CastlingKingTo[queenSide];
        HANDLE_MOVE(makeMove<CASTLING>(from, to));
    }

    return true;
}


// Enumerates all king moves.
template<Color Me, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumerateKingMoves(const Position &pos, Square from, const Handler& handler) {
    Bitboard dest = attacks<KING>(from) & ~pos.getPiecesBB(Me) & ~pos.threatened();

    // For quiet moves, do not add captures.
    if constexpr (MGType == QUIET_MOVES) dest &= ~pos.getPiecesBB(~Me);
    // For tactical moves, add captures.
    if constexpr (MGType == TACTICAL_MOVES) dest &= pos.getPiecesBB(~Me);

    bitloop(dest) {
        Square to = bitscan(dest);
        HANDLE_MOVE(makeMove(from, to));
    }

    return true;
}


// Enumerate all knight moves.
template<Color Me, bool InCheck, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumerateKnightMoves(const Position &pos, Bitboard source, const Handler& handler) {

    // Pinned knights can never move: their moves are both orthogonal *and* diagonal, so filter
    // them out immediately.
    // https://lichess.org/editor/5k2/8/8/5b2/8/3N4/2K5/8_w_-_-_0_1?color=white
    // https://lichess.org/editor/5k2/2r5/8/8/8/2N5/2K5/8_w_-_-_0_1?color=white
    Bitboard knights = source & ~(pos.pinDiag() | pos.pinOrtho());

    bitloop(knights) {
        const Square from = bitscan(knights);
        Bitboard dest = attacks<KNIGHT>(from) & ~pos.getPiecesBB(Me);

        if constexpr (InCheck) dest &= pos.checkMask();
        if constexpr (MGType == TACTICAL_MOVES) dest &= pos.getPiecesBB(~Me);
        if constexpr (MGType == QUIET_MOVES) dest &= ~pos.getPiecesBB(~Me);

        bitloop(dest) {
            Square to = bitscan(dest);
            HANDLE_MOVE(makeMove(from, to));
        }
    }

    return true;
}


// Enumerate all bishop + diagonal queen moves.
template<Color Me, bool InCheck, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumerateBishopSliderMoves(const Position &pos, Bitboard source, const Handler& handler) {
    const Bitboard oppPiecesBB = pos.getPiecesBB(~Me);

    // Orthogonally pinned bishops + queens cannot move (diagonally),
    // as that would make them get out of the pinmask.
    // https://lichess.org/editor/8/4k3/8/2K1B1r1/8/8/8/8_w_-_-_0_1?color=white
    const Bitboard bqCanMove = source & ~pos.pinOrtho();

    Bitboard pieces;

    // Non-pinned bishop + queen
    pieces = bqCanMove & ~pos.pinDiag();
    bitloop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<BISHOP>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me);

        if constexpr (InCheck) dest &= pos.checkMask();
        if constexpr (MGType == TACTICAL_MOVES) dest &= oppPiecesBB;
        if constexpr (MGType == QUIET_MOVES)    dest &= ~oppPiecesBB;

        bitloop(dest) {
            Square to = bitscan(dest);
            HANDLE_MOVE(makeMove(from, to));
        }
    }

    // Pinned bishop + queen
    pieces = bqCanMove & pos.pinDiag();
    bitloop(pieces) {
        Square from = bitscan(pieces);

        // Diagonally pinned bishops + queens can move, but only within the pinmask.
        // https://lichess.org/editor/8/4k1b1/8/4B3/8/2K5/8/8_w_-_-_0_1?color=white
        Bitboard dest = attacks<BISHOP>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me) & pos.pinDiag();

        if constexpr (InCheck) dest &= pos.checkMask();
        if constexpr (MGType == TACTICAL_MOVES) dest &= oppPiecesBB;
        if constexpr (MGType == QUIET_MOVES) dest &= ~oppPiecesBB;

        bitloop(dest) {
            Square to = bitscan(dest);
            HANDLE_MOVE(makeMove(from, to));
        }
    }

    return true;
}


// Enumerate all rook + orthogonal queen moves.
template<Color Me, bool InCheck, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumerateRookSliderMoves(const Position &pos, Bitboard source, const Handler& handler) {
    const Bitboard oppPiecesBB = pos.getPiecesBB(~Me);

    // Diagonally pinned rooks + queens cannot move (orthogonally),
    // as that would make them get out of the pinmask.
    // https://lichess.org/editor/8/4k1b1/8/4R3/8/2K5/8/8_w_-_-_0_1?color=white
    const Bitboard bqCanMove = source & ~pos.pinDiag();

    Bitboard pieces;

    // Non-pinned rook + queen
    pieces = bqCanMove & ~pos.pinOrtho();
    bitloop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<ROOK>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me);

        if constexpr (InCheck) dest &= pos.checkMask();
        if constexpr (MGType == TACTICAL_MOVES) dest &= oppPiecesBB;
        if constexpr (MGType == QUIET_MOVES) dest &= ~oppPiecesBB;

        bitloop(dest) {
            Square to = bitscan(dest);
            HANDLE_MOVE(makeMove(from, to));
        }
    }

    // Pinned rook + queen
    pieces = bqCanMove & pos.pinOrtho();
    bitloop(pieces) {
        Square from = bitscan(pieces);
        // Orthogonally pinned rooks + queens can move, but only within the pinmask.
        // https://lichess.org/editor/8/4k3/8/2K1R1r1/8/8/8/8_w_-_-_0_1?color=white
        Bitboard dest = attacks<ROOK>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me) & pos.pinOrtho();

        if constexpr (InCheck) dest &= pos.checkMask();
        if constexpr (MGType == TACTICAL_MOVES) dest &= oppPiecesBB;
        if constexpr (MGType == QUIET_MOVES) dest &= ~oppPiecesBB;

        bitloop(dest) {
            Square to = bitscan(dest);
            HANDLE_MOVE(makeMove(from, to));
        }
    }

    return true;
}


// Enumerates all legal moves within and calls the handler callback on each legal move found.
template<Color Me, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumerateLegalMoves(const Position &pos, const Handler& handler) {
    switch(pos.nCheckers()) {

        case 0:
            // No pieces checking us: we can make all moves
            ENUMERATE_MOVES(enumeratePawnMoves<Me, false, MGType, Handler>(pos, pos.getPiecesBB(Me, PAWN), handler));
            ENUMERATE_MOVES(enumerateKnightMoves<Me, false, MGType, Handler>(pos, pos.getPiecesBB(Me, KNIGHT), handler));
            ENUMERATE_MOVES(enumerateBishopSliderMoves<Me, false, MGType, Handler>(pos, pos.getPiecesBB(Me, BISHOP, QUEEN), handler));
            ENUMERATE_MOVES(enumerateRookSliderMoves<Me, false, MGType, Handler>(pos, pos.getPiecesBB(Me, ROOK, QUEEN), handler));
            if constexpr (MGType & QUIET_MOVES) ENUMERATE_MOVES(enumerateCastlingMoves<Me, Handler>(pos, handler));
            ENUMERATE_MOVES(enumerateKingMoves<Me, MGType, Handler>(pos, pos.getKingSquare(Me), handler));

            return true;


        case 1:
            // One piece checking us: we can make all moves in the checkmask
            ENUMERATE_MOVES(enumeratePawnMoves<Me, true, ALL_MOVES, Handler>(pos, pos.getPiecesBB(Me, PAWN), handler));
            ENUMERATE_MOVES(enumerateKnightMoves<Me, true, ALL_MOVES, Handler>(pos, pos.getPiecesBB(Me, KNIGHT), handler));
            ENUMERATE_MOVES(enumerateBishopSliderMoves<Me, true, ALL_MOVES, Handler>(pos, pos.getPiecesBB(Me, BISHOP, QUEEN), handler));
            ENUMERATE_MOVES(enumerateRookSliderMoves<Me, true, ALL_MOVES, Handler>(pos, pos.getPiecesBB(Me, ROOK, QUEEN), handler));
            ENUMERATE_MOVES(enumerateKingMoves<Me, ALL_MOVES, Handler>(pos, pos.getKingSquare(Me), handler));

            return true;


        default: //case 2:
            // Two pieces checking us: we can't block the check or take both pieces: we can only move the king
            ENUMERATE_MOVES(enumerateKingMoves<Me, ALL_MOVES, Handler>(pos, pos.getKingSquare(Me), handler));

            return true;
    }
}


// Enumerates all legal moves within and calls the handler callback on each legal move found.
template<MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumerateLegalMoves(const Position &pos, const Handler& handler) {
    return pos.getSideToMove() == WHITE
            ? enumerateLegalMoves<WHITE, MGType, Handler>(pos, handler)
            : enumerateLegalMoves<BLACK, MGType, Handler>(pos, handler);
}

#undef ENUMERATE_MOVES
#undef HANDLE_MOVE

} // namespace Movegen

} // namespace Atom

#endif
