#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "types.h"
#include "position.h"

#define ENUMERATE_MOVES(...) if (!__VA_ARGS__) return false
#define HANDLE_MOVE(...)     if (!handler(__VA_ARGS__)) return false

using MoveList = vector<Move, MAX_MOVE, uint8_t>;

enum MoveGenType {
    QUIET_MOVES = 1,
    TACTICAL_MOVES = 2,
    ALL_MOVES = QUIET_MOVES | TACTICAL_MOVES,
};

template<Side Me, PieceType PromotionType, typename Handler>
inline bool enumeratePromotion(Square from, Square to, const Handler& handler) {
    HANDLE_MOVE(makeMove<PROMOTION>(from, to, PromotionType));

    return true;
}

template<Side Me, MoveGenType MGType = ALL_MOVES, typename Handler>
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

template<Side Me, bool InCheck, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumeratePawnPromotionMoves(const Position &pos, U64 source, const Handler& handler) {
    constexpr Side Opp = ~Me;
    constexpr U64 Rank7 = (Me == WHITE) ? RANK_7_BB : RANK_2_BB;
    constexpr Direction Up = (Me == WHITE) ? UP : DOWN;
    constexpr Direction UpLeft = (Me == WHITE) ? UP_LEFT : DOWN_RIGHT;
    constexpr Direction UpRight = (Me == WHITE) ? UP_RIGHT : DOWN_LEFT;

    U64 emptyBB = pos.getEmptyBB();
    U64 pinOrtho = pos.pinOrtho();
    U64 pinDiag = pos.pinDiag();
    U64 checkMask = pos.checkMask();

    U64 pawnsCanPromote = source & Rank7;

    if (pawnsCanPromote) {
        // Capture Promotion
        {
            U64 pawns = pawnsCanPromote & ~pinOrtho;
            U64 capLPromotions = (shift<UpLeft>(pawns & ~pinDiag)  | (shift<UpLeft>(pawns & pinDiag) & pinDiag)) & pos.getPiecesBB(Opp);
            U64 capRPromotions = (shift<UpRight>(pawns & ~pinDiag) | (shift<UpRight>(pawns & pinDiag) & pinDiag)) & pos.getPiecesBB(Opp);

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
            U64 pawns = pawnsCanPromote & ~pinDiag;
            U64 quietPromotions = (shift<Up>(pawns & ~pinOrtho) | (shift<Up>(pawns & pinOrtho) & pinOrtho)) & emptyBB;

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

template<Side Me, bool InCheck, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumeratePawnEnpassantMoves(const Position &pos, U64 source, const Handler& handler) {
    constexpr Side Opp = ~Me;

    U64 pinOrtho = pos.pinOrtho();
    U64 pinDiag = pos.pinDiag();
    U64 checkMask = pos.checkMask();

    // Enpassant
    if (pos.getEpSquare() != NO_SQUARE) {

        U64 epcaptured = sq_to_bb(pos.getEpSquare() - pawnDirection(Me));
        U64 enpassants = pawnAttacks(Opp, pos.getEpSquare()) & source & ~pinOrtho;

        if constexpr (InCheck) {
            if (!(pos.checkers() & epcaptured)) {
                enpassants &= checkMask;
            }
        }

        bitloop(enpassants) {
            Square from = bitscan(enpassants);
            U64 epRank = sq_to_bb(rankOf(from));
            if ( (sq_to_bb(from) & pinDiag) && !(sq_to_bb(pos.getEpSquare()) & pinDiag))
                continue;
            if (!((epRank & pos.getPiecesBB(Me, KING)) && (epRank & pos.getPiecesBB(Opp, ROOK, QUEEN))) ||
                !(attacks<ROOK>(pos.getKingSquare(Me), pos.getPiecesBB() ^ sq_to_bb(from) ^ epcaptured) & pos.getPiecesBB(Opp, ROOK, QUEEN))) {
                Square to = pos.getEpSquare();
                HANDLE_MOVE(makeMove<EN_PASSANT>(from, to));
            }
        }
    }

    return true;
}

template<Side Me, bool InCheck, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumeratePawnNormalMoves(const Position &pos, U64 source, const Handler& handler) {
    constexpr Side Opp = ~Me;
    constexpr U64 Rank3 = (Me == WHITE) ? RANK_3_BB : RANK_6_BB;
    constexpr U64 Rank7 = (Me == WHITE) ? RANK_7_BB : RANK_2_BB;
    constexpr Direction Up = (Me == WHITE) ? UP : DOWN;
    constexpr Direction UpLeft = (Me == WHITE) ? UP_LEFT : DOWN_RIGHT;
    constexpr Direction UpRight = (Me == WHITE) ? UP_RIGHT : DOWN_LEFT;

    U64 emptyBB = pos.getEmptyBB();
    U64 pinOrtho = pos.pinOrtho();
    U64 pinDiag = pos.pinDiag();
    U64 checkMask = pos.checkMask();

    // Single & Double Push
    if constexpr (MGType & QUIET_MOVES) {
        U64 pawns = source & ~Rank7 & ~pinDiag;
        U64 singlePushes = (shift<Up>(pawns & ~pinOrtho) | (shift<Up>(pawns & pinOrtho) & pinOrtho)) & emptyBB;
        U64 doublePushes = shift<Up>(singlePushes & Rank3) & emptyBB;

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
        U64 pawns = source & ~Rank7 & ~pinOrtho;
        U64 capL = (shift<UpLeft>(pawns & ~pinDiag)  | (shift<UpLeft>(pawns & pinDiag) & pinDiag)) & pos.getPiecesBB(Opp);
        U64 capR = (shift<UpRight>(pawns & ~pinDiag) | (shift<UpRight>(pawns & pinDiag) & pinDiag)) & pos.getPiecesBB(Opp);

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

template<Side Me, bool InCheck, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumeratePawnMoves(const Position &pos, U64 source, const Handler& handler) {
    ENUMERATE_MOVES(enumeratePawnNormalMoves<Me, InCheck, MGType, Handler>(pos, source, handler));
    ENUMERATE_MOVES(enumeratePawnPromotionMoves<Me, InCheck, MGType, Handler>(pos, source, handler));

    if constexpr (MGType & TACTICAL_MOVES) {
        ENUMERATE_MOVES(enumeratePawnEnpassantMoves<Me, InCheck, MGType, Handler>(pos, source, handler));
    }

    return true;
}

template<Side Me, typename Handler>
inline bool enumerateCastlingMoves(const Position &pos, const Handler& handler) {
    Square kingSquare = pos.getKingSquare(Me);

    for (auto cr : {Me & KING_SIDE, Me & QUEEN_SIDE}) {
        if (pos.canCastle(cr) && pos.isEmpty(CastlingPath[cr]) && !(pos.threatened() & CastlingKingPath[cr])) {
            Square from = kingSquare;
            Square to = CastlingKingTo[cr];
            HANDLE_MOVE(makeMove<CASTLING>(from, to));
        }
    }

    return true;
}

template<Side Me, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumerateKingMoves(const Position &pos, Square from, const Handler& handler) {
    U64 dest = attacks<KING>(from) & ~pos.getPiecesBB(Me) & ~pos.threatened();

    if constexpr (MGType == TACTICAL_MOVES) dest &= pos.getPiecesBB(~Me);
    if constexpr (MGType == QUIET_MOVES) dest &= ~pos.getPiecesBB(~Me);

    bitloop(dest) {
        Square to = bitscan(dest);
        HANDLE_MOVE(makeMove(from, to));
    }

    return true;
}

template<Side Me, bool InCheck, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumerateKnightMoves(const Position &pos, U64 source, const Handler& handler) {
    U64 pieces = source & ~(pos.pinDiag() | pos.pinOrtho());

    bitloop(pieces) {
        Square from = bitscan(pieces);
        U64 dest = attacks<KNIGHT>(from) & ~pos.getPiecesBB(Me);

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

template<Side Me, bool InCheck, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumerateBishopSliderMoves(const Position &pos, U64 source, const Handler& handler) {
    U64 pieces;
    U64 oppPiecesBB = pos.getPiecesBB(~Me);

    // Non-pinned Bishop & Queen
    pieces = source & ~pos.pinOrtho() & ~pos.pinDiag();
    bitloop(pieces) {
        Square from = bitscan(pieces);
        U64 dest = attacks<BISHOP>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me);

        if constexpr (InCheck) dest &= pos.checkMask();
        if constexpr (MGType == TACTICAL_MOVES) dest &= oppPiecesBB;
        if constexpr (MGType == QUIET_MOVES)    dest &= ~oppPiecesBB;

        bitloop(dest) {
            Square to = bitscan(dest);
            HANDLE_MOVE(makeMove(from, to));
        }
    }

    // Pinned Bishop & Queen
    pieces = source & ~pos.pinOrtho() & pos.pinDiag();
    bitloop(pieces) {
        Square from = bitscan(pieces);
        U64 dest = attacks<BISHOP>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me) & pos.pinDiag();

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

template<Side Me, bool InCheck, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumerateRookSliderMoves(const Position &pos, U64 source, const Handler& handler) {
    U64 pieces;
    U64 oppPiecesBB = pos.getPiecesBB(~Me);

    // Non-pinned Rook & Queen
    pieces = source & ~pos.pinDiag() & ~pos.pinOrtho();
    bitloop(pieces) {
        Square from = bitscan(pieces);
        U64 dest = attacks<ROOK>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me);

        if constexpr (InCheck) dest &= pos.checkMask();
        if constexpr (MGType == TACTICAL_MOVES) dest &= oppPiecesBB;
        if constexpr (MGType == QUIET_MOVES) dest &= ~oppPiecesBB;

        bitloop(dest) {
            Square to = bitscan(dest);
            HANDLE_MOVE(makeMove(from, to));
        }
    }

    // Pinned Rook & Queen
    pieces = source & ~pos.pinDiag() & pos.pinOrtho();
    bitloop(pieces) {
        Square from = bitscan(pieces);
        U64 dest = attacks<ROOK>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me) & pos.pinOrtho();

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

template<Side Me, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumerateLegalMoves(const Position &pos, const Handler& handler) {
    switch(pos.nbCheckers()) {
        case 0:
            ENUMERATE_MOVES(enumeratePawnMoves<Me, false, MGType, Handler>(pos, pos.getPiecesBB(Me, PAWN), handler));
            ENUMERATE_MOVES(enumerateKnightMoves<Me, false, MGType, Handler>(pos, pos.getPiecesBB(Me, KNIGHT), handler));
            ENUMERATE_MOVES(enumerateBishopSliderMoves<Me, false, MGType, Handler>(pos, pos.getPiecesBB(Me, BISHOP, QUEEN), handler));
            ENUMERATE_MOVES(enumerateRookSliderMoves<Me, false, MGType, Handler>(pos, pos.getPiecesBB(Me, ROOK, QUEEN), handler));
            if constexpr (MGType & QUIET_MOVES) ENUMERATE_MOVES(enumerateCastlingMoves<Me, Handler>(pos, handler));
            ENUMERATE_MOVES(enumerateKingMoves<Me, MGType, Handler>(pos, pos.getKingSquare(Me), handler));

            return true;
        case 1:
            ENUMERATE_MOVES(enumeratePawnMoves<Me, true, ALL_MOVES, Handler>(pos, pos.getPiecesBB(Me, PAWN), handler));
            ENUMERATE_MOVES(enumerateKnightMoves<Me, true, ALL_MOVES, Handler>(pos, pos.getPiecesBB(Me, KNIGHT), handler));
            ENUMERATE_MOVES(enumerateBishopSliderMoves<Me, true, ALL_MOVES, Handler>(pos, pos.getPiecesBB(Me, BISHOP, QUEEN), handler));
            ENUMERATE_MOVES(enumerateRookSliderMoves<Me, true, ALL_MOVES, Handler>(pos, pos.getPiecesBB(Me, ROOK, QUEEN), handler));
            ENUMERATE_MOVES(enumerateKingMoves<Me, ALL_MOVES, Handler>(pos, pos.getKingSquare(Me), handler));

            return true;
        default: //case 2:
            // If we are in double check only king moves are allowed
            ENUMERATE_MOVES(enumerateKingMoves<Me, ALL_MOVES, Handler>(pos, pos.getKingSquare(Me), handler));

            return true;
    }
}

template<MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumerateLegalMoves(const Position &pos, const Handler& handler) {
    return pos.getSideToMove() == WHITE
            ? enumerateLegalMoves<WHITE, MGType, Handler>(pos, handler)
            : enumerateLegalMoves<BLACK, MGType, Handler>(pos, handler);
}

#endif
