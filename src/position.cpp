
#include "position.h"
#include "bitboard.h"
#include "movegen.h"
#include "tt.h"
#include "types.h"
#include "zobrist.h"
#include "uci.h"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <sstream>

namespace Atom {

const std::string_view PIECE_TO_CHAR(" PNBRQK  pnbrqk");


// Updates all the required bitboards for the current
// position.
inline void Position::updateBitboards() {
    sideToMove == WHITE ? updateBitboards<WHITE>() : updateBitboards<BLACK>(); 
}


// Updates all the required bitboards for the current
// position. Call the non-templated version of this function instead.
template<Color Me>
inline void Position::updateBitboards() {
    updateThreatened<Me>();
    updateCheckers<Me>();
    checkers() ? updatePinsAndCheckMask<Me, true>() : updatePinsAndCheckMask<Me, false>();
}


// Updates the threatened squares for the current position.
// These are all the squares that the opponent's pieces can
// attack.
template<Color Me>
inline void Position::updateThreatened() {
    constexpr Color Opp = ~Me;
    const Bitboard occ = getPiecesBB() ^ getPiecesBB(Me, KING);
    Bitboard threatened, enemies;

    // Pawns
    threatened = allPawnAttacks<Opp>(getPiecesBB(Opp, PAWN));

    // Knights
    enemies = getPiecesBB(Opp, KNIGHT);
    loopOverBits(enemies, [&](Square s) {
        threatened |= KNIGHT_MOVE[s];
    });

    // Bishops + queens
    enemies = getPiecesBB(Opp, BISHOP, QUEEN);
    loopOverBits(enemies, [&](Square s) {
        threatened |= attacks<BISHOP>(s, occ);
    });

    // Rooks + queens
    enemies = getPiecesBB(Opp, ROOK, QUEEN);
    loopOverBits(enemies, [&](Square s) {
        threatened |= attacks<ROOK>(s, occ);
    });

    // King
    threatened |= attacks<KING>(getKingSquare(Opp));

    state->attacked = threatened;
}


// Updates the pinmask and checkmask for the current position.
//
// The pinmask contains all squares between the king, pinned piece,
// and pinning piece, including the pinning piece.
//
//       . . . . . . . .     0 0 0 0 0 0 0 0
//       . . . . . . . .     0 0 0 0 0 0 0 0
//       . . . . . . . .     0 0 0 0 0 0 0 0
//       . K . . P . . q     0 0 1 1 1 1 1 1
//       . . . . . . . .     0 1 0 0 0 0 0 0
//       . P . . . . . .     0 1 0 0 0 0 0 0
//       . . . . . . . .     0 1 0 0 0 0 0 0
//       . r . . . . . .     0 1 0 0 0 0 0 0
//
// The checkmask contains all squares between the king and the checking
// piece, including the checking piece. If the checking piece is a knight, then
// the checkmask only includes that square.
//
//       . . . . . . . .     0 0 0 0 0 0 0 0
//       . . . . . . . .     0 0 0 0 0 0 0 0
//       . . . . . . . .     0 0 0 0 0 0 0 0
//       . K . . . . . .     0 0 0 0 0 0 0 0
//       . . . . . . . .     0 0 1 0 0 0 0 0
//       . . . . . . . .     0 0 0 1 0 0 0 0
//       . . . . . . . .     0 0 0 0 1 0 0 0
//       . . . . . b . .     0 0 0 0 0 1 0 0
template<Color Me, bool InCheck>
inline void Position::updatePinsAndCheckMask() {
    constexpr Color Opp = ~Me;
    const Square ksq       = getKingSquare(Me);
    const Bitboard opp_occ = getPiecesBB(Opp);
    const Bitboard my_occ  = getPiecesBB(Me);
    Bitboard pinDiag = EMPTY, pinOrtho = EMPTY;
    Bitboard checkmask, between;

    // Pawns and knights
    if constexpr (InCheck) {
        checkmask = (pawnAttacks<Me>(ksq) & getPiecesBB(Opp, PAWN))
                  | (attacks<KNIGHT>(ksq) & getPiecesBB(Opp, KNIGHT));
    }

    // Bishops and queens
    Bitboard pinners = (attacks<BISHOP>(ksq, opp_occ) & getPiecesBB(Opp, BISHOP, QUEEN));
    loopOverBits(pinners, [&](Square s) {
        between = BETWEEN_BB[ksq][s];

        switch(popcount(between & my_occ)) {
            case 0: if constexpr (InCheck) checkmask |= between | sqToBB(s); break;
            case 1: pinDiag |= between | sqToBB(s);
        }
    });

    // Rooks and queens
    pinners = (attacks<ROOK>(ksq, opp_occ) & getPiecesBB(Opp, ROOK, QUEEN));
    loopOverBits(pinners, [&](Square s) {
        between = BETWEEN_BB[ksq][s];

        switch(popcount(between & my_occ)) {
            case 0: if constexpr (InCheck) checkmask |= between | sqToBB(s); break;
            case 1: pinOrtho |= between | sqToBB(s);
        }
    });

    state->pinDiag = pinDiag;
    state->pinOrtho = pinOrtho;
    if constexpr (InCheck) state->checkMask = checkmask;
}


// Updates the checkers for the current position.
// This is a mask that simply contains 1s for all pieces
// checking the king, else 0s. For all legal positions,
// this will have < 2 bits set.
//
//       . . . . . . . .     0 0 0 0 0 0 0 0
//       . . . . . . . .     0 0 0 0 0 0 0 0
//       . . . . . . . .     0 0 0 0 0 0 0 0
//       . K . . . . . .     0 0 0 0 0 0 0 0
//       . . . n . . . .     0 0 0 1 0 0 0 0
//       . . . . . . . .     0 0 0 0 0 0 0 0
//       . . . . . . . .     0 0 0 0 0 0 0 0
//       . . . . . b . .     0 0 0 0 0 1 0 0
template<Color Me>
inline void Position::updateCheckers() {
    const Square ksq = getKingSquare(Me);
    constexpr Color Opp = ~Me;
    const Bitboard occ = getPiecesBB();

    state->checkers = ((pawnAttacks<Me>(ksq) & getPiecesBB(Opp, PAWN))
        | (attacks<KNIGHT>(ksq) & getPiecesBB(Opp, KNIGHT))
        | (attacks<BISHOP>(ksq, occ) & getPiecesBB(Opp, BISHOP, QUEEN))
        | (attacks<ROOK>(ksq, occ) & getPiecesBB(Opp, ROOK, QUEEN))
    );
}


// Computes the hash for the current position
Key Position::computeHash() const {
    Key hash = 0;

    Piece p;
    Bitboard pieces = getPiecesBB();

    loopOverBits(pieces, [&](Square s) {
        p = getPieceAt(s);
        hash ^= Zobrist::keys[p][s];
    });

    hash ^= Zobrist::castlingKeys[getCastlingRights()];
    if (getEpSquare() != SQ_NONE) hash ^= Zobrist::enpassantKeys[fileOf(getEpSquare())];
    if (getSideToMove() == BLACK) hash ^= Zobrist::sideToMoveKey;

    return hash;
}


// Computes the pawn key for the position. This is used for
// move picking.
Key Position::computePawnKey() const {
    Key pawnKey = Zobrist::noPawnsKey;

    Piece p;
    Bitboard allPawns = getPiecesBB(PAWN);

    loopOverBits(allPawns, [&](Square s) {
        p = getPieceAt(s);
        pawnKey ^= Zobrist::keys[p][s];
    });

    return pawnKey;
}


//
// Returns a bitboard with 1s set on all squares where there is a piece
// that can attack the given square.
// For example: (# = given square)
//       . . . . Q . . .     0 0 0 0 1 0 0 0
//       . . . . . . . .     0 0 0 0 0 0 0 0
//       . . N . . . . .     0 0 1 0 0 0 0 0
//       . . . K # k . .     0 0 0 1 0 1 0 0
//       . . n . . . . .     0 0 1 0 0 0 0 0
//       . . . . . . b .     0 0 0 0 0 0 1 0
//       . B . . . . . .     0 1 0 0 0 0 0 0
//       . . . . q . . .     0 0 0 0 1 0 0 0
inline Bitboard Position::getAttackersTo(const Square s, const Bitboard occ) const {
    return (pawnAttacks<BLACK>(s) & getPiecesBB(WHITE, PAWN))
         | (pawnAttacks<WHITE>(s) & getPiecesBB(BLACK, PAWN))
         | (attacks<KNIGHT>(s) & getPiecesBB(KNIGHT))
         | (attacks<ROOK>(s, occ) & getPiecesBB(ROOK, QUEEN))
         | (attacks<BISHOP>(s, occ) & getPiecesBB(BISHOP, QUEEN))
         | (attacks<KING>(s) & getPiecesBB(KING));
}


char pieceToChar(Piece p) {
    return PIECE_TO_CHAR[p];
}


Piece charToPiece(char c) {
    return Piece(PIECE_TO_CHAR.find(c));
}


// Initializes the position and sets it to the starting position.
//       r n b q k b n r
//       p p p p p p p p
//       . . . . . . . .
//       . . . . . . . .
//       . . . . . . . .
//       . . . . . . . .
//       P P P P P P P P
//       R N B Q K B N R
Position::Position() {
    history = new BoardState[MAX_HISTORY];
    state = history;
    setFromFEN(STARTPOS_FEN);
}


// Creates a copy of another position.
Position::Position(const Position &other) {
    history = new BoardState[MAX_HISTORY];
    std::memcpy(this, &other, sizeof(Position));
    this->state = this->history + (other.state - other.history);
}


// Copy assignment operator
Position& Position::operator=(const Position &other) {
    if (this == &other) return *this; // Self assignment check
    std::memcpy(this, &other, sizeof(Position));
    this->state = this->history + (other.state - other.history);

    return *this;
}


// Destructor
Position::~Position() {
    delete[] history;
}


// Resets the current position to empty.
void Position::reset() {
    state = &history[0];

    state->fiftyMoveRule = 0;
    state->halfMoves = 0;
    state->epSquare = SQ_NONE;
    state->castlingRights = NO_CASTLING;
    state->move = MOVE_NONE;

    for(int i = 0; i < SQUARE_NB; i++) pieces[i]   = NO_PIECE;
    for(int i = 0; i < PIECE_NB;  i++) piecesBB[i] = EMPTY;

    sideBB[WHITE] = sideBB[BLACK] = EMPTY;
    sideToMove = WHITE;
}


// Sets the current position according to a given FEN notation.
// https://www.chessprogramming.org/Forsyth-Edwards_Notation
bool Position::setFromFEN(const std::string &fen) {
    reset();

    std::istringstream parser(fen);
    std::string token;

    // Process piece placement from FEN
    parser >> std::skipws >> token;
    int file = 0, rank = 7;
    for (char c : token) {
        if (c == '/') {
            // Move to the next rank
            rank--;
            file = 0;
        } else if (std::isdigit(c)) {
            // Skip the specified number of files
            file += c - '0';
        } else {
            // Place the piece on the board
            Piece piece = charToPiece(c);
            Square s = createSquare(File(file), Rank(rank));

            if (!isValidPiece(piece) || !isValidSq(s)) {
                std::cout << "Invalid FEN (invalid piece / square)" << std::endl;
                reset();
                return false;
            }

            colorOf(piece) == WHITE ? setPiece<WHITE>(s, piece) : setPiece<BLACK>(s, piece);
            file++;
        }
    }
    if (file != 8 || rank != 0) {
        std::cout << "Invalid FEN (incorrect piece placement)" << std::endl;
    }

    // Process side to move
    parser >> std::skipws >> token;
    switch (token[0]) {
        case 'w': sideToMove = WHITE; break;
        case 'b': sideToMove = BLACK; break;
        default:
            reset();
            return false;
    }

    // Process castling rights
    parser >> std::skipws >> token;
    for (char c : token) {
        switch (c) {
            case 'K': state->castlingRights |= WHITE_OO;  break;
            case 'Q': state->castlingRights |= WHITE_OOO; break;
            case 'k': state->castlingRights |= BLACK_OO;  break;
            case 'q': state->castlingRights |= BLACK_OOO; break;
            case '-': break;
            default:
                reset();
                return false;
        }
    }

    // Process en passant square
    parser >> std::skipws >> token;
    state->epSquare = Uci::parseSquare(token);
    if (state->epSquare != SQ_NONE && !isValidSq(state->epSquare)) {
        reset();
        return false;
    }

    // Process fifty-move rule counter
    parser >> std::skipws >> state->fiftyMoveRule;
    if (state->fiftyMoveRule < 0) {
        reset();
        return false;
    }

    // Process full move counter
    parser >> std::skipws >> state->halfMoves;
    state->halfMoves = std::max(2 * (state->halfMoves - 1), 0) + (sideToMove == BLACK);
    if (state->halfMoves < 0) {
        reset();
        return false;
    }

    // Update bitboards and hash
    updateBitboards();
    this->state->hash = computeHash();

    return true;
}


// Returns the current FEN of the position.
// https://www.chessprogramming.org/Forsyth-Edwards_Notation
std::string Position::fen() const {
    std::ostringstream ss;
    int empty;

    // Position part of fen
    for (int r = RANK_8; r >= RANK_1; --r) {
        for (int f = FILE_A; f <= FILE_H; ++f) {
            empty = 0;

            while (f <= FILE_H && isEmpty(createSquare(File(f), Rank(r)))) {
                ++empty;
                ++f;
            }

            if (empty) ss << empty;
            if (f <= FILE_H) ss << PIECE_TO_CHAR[getPieceAt(createSquare(File(f), Rank(r)))];
        }

        if (r > RANK_1) ss << '/';
    }

    // Color to move
    ss << (sideToMove == WHITE ? " w " : " b ");

    // Castling rights
    if (canCastle(WHITE_OO))  ss << 'K';
    if (canCastle(WHITE_OOO)) ss << 'Q';
    if (canCastle(BLACK_OO))  ss << 'k';
    if (canCastle(BLACK_OO))  ss << 'q';
    if (!canCastle(ALL_CASTLING))    ss << '-';

    // En passant square
    ss << (getEpSquare() == SQ_NONE ? " - " : " " + Uci::formatSquare(getEpSquare()) + " ");

    // Halfmove + fullmove clock
    ss << getHalfMoveClock() << " " << getFullMoves();

    return ss.str();
}


// Returns a string containing a visualization of the current position.
std::string Position::printable() const {
    std::ostringstream ss;
    ss << "   +---+---+---+---+---+---+---+---+" << std::endl;
    for (int r = RANK_8; r >= RANK_1; --r) {
        ss << " " << r + 1 << " | ";
        for (int f = FILE_A; f <= FILE_H; ++f) {
            ss << PIECE_TO_CHAR[getPieceAt(createSquare(File(f), Rank(r)))];
            ss << (f < FILE_H ? " | " : " | ");
        }
        if (r > RANK_1)
            ss << std::endl << "   +---+--─+---+---+---+---+---+---+" << std::endl;
    }
    ss << std::endl << "   +---+---+---+---+---+---+---+---+" << std::endl;
    ss << std::endl << "     a   b   c   d   e   f   g   h" << std::endl;

    return ss.str();
}


// Adds the piece to the position at the current square
template<Color Me>
inline void Position::setPiece(Square sq, Piece p) {
    Bitboard b = sqToBB(sq);
    pieces[sq] = p;
    sideBB[Me]  |= b;
    piecesBB[p] |= b;
}


// Removes the piece from the given square.
template<Color Me>
inline void Position::unsetPiece(Square sq) {
    Bitboard b = sqToBB(sq);
    Piece p = pieces[sq];
    pieces[sq] = NO_PIECE;
    sideBB[Me]  &= ~b;
    piecesBB[p] &= ~b;
}


// Moves the piece from the "from" square to the "to" square.
// Moves should not be called with this function: use doMove instead.
template<Color Me>
inline void Position::movePiece(Square from, Square to) {
    Bitboard fromTo = from | to;
    Piece p = pieces[from];
    pieces[to] = p;
    pieces[from] = NO_PIECE;
    sideBB[Me]  ^= fromTo;
    piecesBB[p] ^= fromTo;
}


// Makes the given move in the current position and updates the position
// accordingly.
template<Color Me, MoveType Mt>
void Position::doMove(Move m) {
    const Square from = moveFrom(m);
    const Square to = moveTo(m);
    const Piece p = getPieceAt(from);
    const Piece captured = getPieceAt(to);

    uint64_t hash = state->hash;

    // Update the hash to remove en passant square
    hash ^= Zobrist::enpassantKeys[fileOf(state->epSquare) + FILE_NB * (state->epSquare == SQ_NONE)];

    // Copy current state to next state
    BoardState *oldState = state++;
    state->epSquare = SQ_NONE;
    state->castlingRights = oldState->castlingRights;
    state->fiftyMoveRule = oldState->fiftyMoveRule + 1;
    state->halfMoves = oldState->halfMoves + 1;
    state->captured = captured;
    state->move = m;
    state->previous = oldState;

    // NNUE
    state->accumulatorBig.computed[WHITE] = state->accumulatorBig.computed[BLACK] =
        state->accumulatorSmall.computed[WHITE] = state->accumulatorSmall.computed[BLACK] = false;

    DirtyPiece& dp = state->dirtyPiece;

    if constexpr (Mt == MT_NORMAL) {
        // Handle normal moves

        dp.dirty_num = 1;
        dp.piece[0] = p;
        dp.from[0]  = from;
        dp.to[0]    = to;

        if (captured != NO_PIECE) {
            hash ^= Zobrist::keys[captured][to];
            unsetPiece<~Me>(to);
            state->fiftyMoveRule = 0;

            dp.dirty_num = 2;
            dp.piece[1]  = captured;
            dp.from[1]   = to;
            dp.to[1]     = SQ_NONE;

            if (typeOf(captured) == PAWN) {
                state->pawnKey ^= Zobrist::keys[captured][to];
            }
        }

        // Update hashes
        hash ^= Zobrist::keys[p][from] ^ Zobrist::keys[p][to];

        hash ^= Zobrist::castlingKeys[state->castlingRights];
        state->castlingRights &= ~(CastlingRightsMask[from] | CastlingRightsMask[to]);
        hash ^= Zobrist::castlingKeys[state->castlingRights];


        movePiece<Me>(from, to);

        // Special handling for pawn moves
        if (typeOf(p) == PAWN) {
            state->fiftyMoveRule = 0;

            if ((int(from) ^ int(to)) == int(NORTH + NORTH)) {
                const Square epsq = to - pawnDirection(Me);

                if (pawnAttacks<Me>(epsq) & getPiecesBB(~Me, PAWN)) {
                    hash ^= Zobrist::enpassantKeys[fileOf(epsq)];
                    state->epSquare = epsq;
                }
            }

            // Update pawn key
            state->pawnKey ^= Zobrist::keys[p][from] ^ Zobrist::keys[p][to];
        }

    } else if constexpr (Mt == MT_CASTLING) {
        // Handle castling moves
        CastlingRight cr = Me & (to > from ? KING_SIDE : QUEEN_SIDE);
        const Square rookFrom = CastlingRookFrom[cr];
        const Square rookTo = CastlingRookTo[cr];

        dp.dirty_num = 2;

        dp.piece[0] = makePiece(Me, KING);
        dp.from[0]  = from;
        dp.to[0]    = to;

        dp.piece[1] = makePiece(Me, ROOK);
        dp.from[1]  = rookFrom;
        dp.to[1]    = rookTo;

        hash ^= Zobrist::keys[makePiece(Me, KING)][from] ^ Zobrist::keys[makePiece(Me, KING)][to];
        hash ^= Zobrist::keys[makePiece(Me, ROOK)][rookFrom] ^ Zobrist::keys[makePiece(Me, ROOK)][rookTo];

        movePiece<Me>(from, to);
        movePiece<Me>(rookFrom, rookTo);

        // Update castling rights
        hash ^= Zobrist::castlingKeys[state->castlingRights];
        state->castlingRights &= ~(CastlingRightsMask[from] | CastlingRightsMask[to]);
        hash ^= Zobrist::castlingKeys[state->castlingRights];

    } else if constexpr (Mt == MT_PROMOTION) {
        // Handle pawn promotion moves
        const PieceType promotionType = movePromotionType(m);
        const Piece piecePromotedTo = makePiece(Me, promotionType);

        // Don't set dirty num here: we set it later, as it depends on captures
        dp.piece[0] = makePiece(Me, PAWN);
        dp.from[0]  = from;
        dp.to[0]    = SQ_NONE;

        if (captured != NO_PIECE) {
            // Capture promotion
            hash ^= Zobrist::keys[captured][to];
            unsetPiece<~Me>(to);

            // Pawn and captured piece go to none, piecePromotedTo goes to sq, so 3 pieces moved
            dp.dirty_num = 3;

            dp.piece[1]  = captured;
            dp.from[1]   = to;
            dp.to[1]     = SQ_NONE;

            dp.piece[2]  = piecePromotedTo;
            dp.from[2]   = SQ_NONE;
            dp.to[2]     = to;

        } else {
            // Regular promotion
            dp.dirty_num = 2;

            dp.piece[1]  = piecePromotedTo;
            dp.from[1]   = SQ_NONE;
            dp.to[1]     = to;
        }

        hash ^= Zobrist::keys[makePiece(Me, PAWN)][from] ^ Zobrist::keys[piecePromotedTo][to];

        // Update bitboards
        unsetPiece<Me>(from);
        setPiece<Me>(to, piecePromotedTo);

        // Reset fifty move rule
        state->fiftyMoveRule = 0;

        // Update castling rights
        hash ^= Zobrist::castlingKeys[state->castlingRights];
        state->castlingRights &= ~(CastlingRightsMask[from] | CastlingRightsMask[to]);
        hash ^= Zobrist::castlingKeys[state->castlingRights];

        // Update pawn key
        state->pawnKey ^= Zobrist::keys[p][from];

    } else if constexpr (Mt == MT_EN_PASSANT) {
        // Handle en passant captures
        const Square epsq = to - pawnDirection(Me);

        // NNUE
        dp.dirty_num = 2;

        dp.piece[0]  = makePiece(Me, PAWN);
        dp.from[0]   = from;
        dp.to[0]     = to;

        dp.piece[1]  = makePiece(~Me, PAWN);
        dp.from[1]   = epsq;
        dp.to[1]     = SQ_NONE;

        // Update hash
        hash ^= Zobrist::keys[makePiece(~Me, PAWN)][epsq];
        hash ^= Zobrist::keys[makePiece(Me, PAWN)][from] ^ Zobrist::keys[makePiece(Me, PAWN)][to];

        // Update bitboards
        unsetPiece<~Me>(epsq);
        movePiece<Me>(from, to);

        // Update fifty move rule
        state->fiftyMoveRule = 0;

        // Update pawn key
        state->pawnKey ^= Zobrist::keys[makePiece(~Me, PAWN)][epsq] ^ Zobrist::keys[p][from];
    }

    // Update hash for the side to move
    hash ^= Zobrist::sideToMoveKey;
    state->hash = hash;
    sideToMove = ~Me;
    updateBitboards<~Me>();
}


template void Position::doMove<WHITE, MT_NORMAL>(Move m);
template void Position::doMove<WHITE, MT_PROMOTION>(Move m);
template void Position::doMove<WHITE, MT_EN_PASSANT>(Move m);
template void Position::doMove<WHITE, MT_CASTLING>(Move m);
template void Position::doMove<BLACK, MT_NORMAL>(Move m);
template void Position::doMove<BLACK, MT_PROMOTION>(Move m);
template void Position::doMove<BLACK, MT_EN_PASSANT>(Move m);
template void Position::doMove<BLACK, MT_CASTLING>(Move m);


// Undoes the given move in the current position and updates the position
// accordingly.
template<Color Me, MoveType Mt>
void Position::undoMove(Move m) {
    const Square from = moveFrom(m);
    const Square to = moveTo(m);
    const Piece capture = state->captured;

    assert(getPieceAt(from) == NO_PIECE || moveTypeOf(m) == MT_CASTLING);

    state--;
    sideToMove = Me;

    if constexpr (Mt == MT_NORMAL) {
        movePiece<Me>(to, from);

        if (capture != NO_PIECE) {
            setPiece<~Me>(to, capture);
        }
    }

    else if constexpr (Mt == MT_CASTLING) {
        const CastlingRight cr = Me & (to > from ? KING_SIDE : QUEEN_SIDE);
        const Square rookFrom = CastlingRookFrom[cr];
        const Square rookTo = CastlingRookTo[cr];

        movePiece<Me>(to, from);
        movePiece<Me>(rookTo, rookFrom);
    }

    else if constexpr (Mt == MT_PROMOTION){
        unsetPiece<Me>(to);
        setPiece<Me>(from, makePiece(Me, PAWN));

        if (capture != NO_PIECE) {
            setPiece<~Me>(to, capture);
        }
    }

    else if constexpr (Mt == MT_EN_PASSANT) {
        movePiece<Me>(to, from);

        const Square epsq = to - pawnDirection(Me);
        setPiece<~Me>(epsq, makePiece(~Me, PAWN));

    }
}

template void Position::undoMove<WHITE, MT_NORMAL>(Move m);
template void Position::undoMove<WHITE, MT_PROMOTION>(Move m);
template void Position::undoMove<WHITE, MT_EN_PASSANT>(Move m);
template void Position::undoMove<WHITE, MT_CASTLING>(Move m);
template void Position::undoMove<BLACK, MT_NORMAL>(Move m);
template void Position::undoMove<BLACK, MT_PROMOTION>(Move m);
template void Position::undoMove<BLACK, MT_EN_PASSANT>(Move m);
template void Position::undoMove<BLACK, MT_CASTLING>(Move m);

template<Color Me>
void Position::doNullMove(TranspositionTable& tt) {

    assert(!checkers());

    ++state;

    // Handle NNUE
    state->dirtyPiece.dirty_num = 0;
    state->dirtyPiece.piece[0]  = NO_PIECE;
    state->accumulatorBig.computed[WHITE] = state->accumulatorBig.computed[BLACK] =
        state->accumulatorSmall.computed[WHITE] = state->accumulatorSmall.computed[BLACK] = false;

    // Increment fifty move rule
    ++state->fiftyMoveRule;

    // Reset EP square
    state->epSquare = SQ_NONE;

    // Handle hash
    state->hash ^= Zobrist::enpassantKeys[fileOf(state->epSquare) + FILE_NB * (state->epSquare == SQ_NONE)];
    state->hash ^= Zobrist::sideToMoveKey;

    // Prefetch entry here to save time
    tt.prefetch(hash());

    // Update other bitboards and state info
    sideToMove = ~Me;
    updateThreatened<~Me>();
    state->checkers = EMPTY;
    updatePinsAndCheckMask<~Me, false>();

}

template void Position::doNullMove<WHITE>(TranspositionTable& tt);
template void Position::doNullMove<BLACK>(TranspositionTable& tt);


// Undo a null move from the position
template<Color Me> void Position::undoNullMove() {
    state--;
    sideToMove = Me;
}

template void Position::undoNullMove<WHITE>();
template void Position::undoNullMove<BLACK>();

// Find if a given move is legal
template<Color Me>
bool Position::isLegalMove(Move m) const {
    assert(isValidMove(m));

    const Square from = moveFrom(m);
    const Square to   = moveTo(m);

    assert(colorOf(   getPieceAt(from)) == Me);

    const MoveType mt = moveTypeOf(m);

    if (mt == MT_EN_PASSANT) {
        const Square ksq   = getKingSquare(Me);
        const Square capsq = to - pawnDirection(Me);
        const Bitboard occ = (getPiecesBB() ^ from ^ capsq) | to;

        assert(to == getEpSquare());
        assert(getPieceAt(to) != NO_PIECE);

        return !(attacks<ROOK>(ksq, occ) & getPiecesBB(~Me, QUEEN, ROOK))
            && !(attacks<BISHOP>(ksq, occ) & getPiecesBB(~Me, QUEEN, BISHOP));

    } else if (mt == MT_CASTLING) {
        const CastlingRight kingSide  = Me & KING_SIDE;
        const CastlingRight queenSide = Me & QUEEN_SIDE;
        return ((canCastle(kingSide) && isEmpty(CastlingPath[kingSide]) && !(threatened() & CastlingKingPath[kingSide]))
         || (canCastle(queenSide) && isEmpty(CastlingPath[queenSide]) && !(threatened() & CastlingKingPath[queenSide])));
    }

    if (typeOf(getPieceAt(from)) != KING) {
        // A non-king move is legal if either:
        // 1. it is not pinned
        // 2. it is pinned but remains in the pinmask

        return (
             to & ~(pinOrtho() | pinDiag())
         || (to & pinOrtho() && from & pinOrtho())
         || (to & pinDiag()  && from & pinDiag())
        );
    }

    // King moves are legal if the square is not threatened.
    return !(to & threatened());
}

template bool Position::isLegalMove<WHITE>(Move m) const;
template bool Position::isLegalMove<BLACK>(Move m) const;


// Test if a move is pseudo-legal. Used for transposition table that may have
// race conditions where moves might be corrupted.
template<Color Me>
bool Position::isPseudoLegalMove(const Move m) const {
    const Square from = moveFrom(m);
    const Square to   = moveTo(m);
    if (from == to) return false;

    const Piece pc    = getPieceAt(from);
    const Piece cap   = getPieceAt(to);

    if (pc == NO_PIECE || colorOf(pc) != Me || to & getPiecesBB(Me)) return false;
    if (typeOf(cap) == KING || (cap != NO_PIECE && colorOf(cap) == Me)) return false;

    if (inCheck()) {
        return cap == NO_PIECE ? isInMoveList<Me, true, false>(m, pc) : isInMoveList<Me, true, true>(m, pc);
    }

    return cap == NO_PIECE ? isInMoveList<Me, false, false>(m, pc) : isInMoveList<Me, false, true>(m, pc);
}


template bool Position::isPseudoLegalMove<WHITE>(Move m) const;
template bool Position::isPseudoLegalMove<BLACK>(Move m) const;


template<Color Me, bool InCheck, bool IsCapture>
inline bool Position::isInMoveList(const Move m, const Piece pc) const {

    Square from     = moveFrom(m);
    Bitboard fromBB = sqToBB(from);
    PieceType pt    = typeOf(pc);

    switch (moveTypeOf(m)) {
        case MT_NORMAL:
            if (nCheckers() > 1 && pt != KING) return false;

            switch (pt) {
                case PAWN:
                    return !Movegen::enumeratePawnNormalMoves<Me, InCheck>(*this, fromBB, [m](Move x) { return m != x; } );
                case KNIGHT:
                    return !Movegen::enumerateKnightMoves<Me, InCheck>(*this, fromBB, [m](Move x) { return m != x; } );
                case BISHOP:
                    return !Movegen::enumerateDiagSliderMoves<Me, InCheck>(*this, fromBB, [m](Move x) { return m != x; } );
                case ROOK:
                    return !Movegen::enumerateOrthoSliderMoves<Me, InCheck>(*this, fromBB, [m](Move x) { return m != x; } );
                case QUEEN:
                    return !Movegen::enumerateDiagSliderMoves<Me, InCheck>(*this, fromBB, [m](Move x) { return m != x; } )
                        || !Movegen::enumerateOrthoSliderMoves<Me, InCheck>(*this, fromBB, [m](Move x) { return m != x; } );
                case KING:
                    return !Movegen::enumerateKingMoves<Me>(*this, from, [m](Move x) { return m != x; } );

                default:
                    return false;
            }

        case MT_CASTLING:
            if constexpr (InCheck) return false;
            return !Movegen::enumerateCastlingMoves<Me>(*this, [m](Move x) { return m != x; } );

        case MT_PROMOTION:
            if (nCheckers() > 1 || pt != PAWN) return false;
            return !Movegen::enumeratePawnPromotionMoves<Me, InCheck>(*this, fromBB, [m](Move x) { return m != x; } );

        case MT_EN_PASSANT:
            if (nCheckers() > 1 || pt != PAWN || getEpSquare() != moveTo(m)) return false;
            return !Movegen::enumeratePawnEnpassantMoves<Me, InCheck>(*this, fromBB, [m](Move x) { return m != x; } );

        default:
            return false;
    }
}


// Static exchange evaluation. If all the available trades happen in the
// position, are we winning?
bool Position::see(const Move move, const int threshold) const {

    constexpr int PIECE_VALS[PIECE_TYPE_NB] = {
        0, // No piece
        VALUE_PAWN, VALUE_KNIGHT, VALUE_BISHOP, VALUE_ROOK, VALUE_QUEEN,
        0, // King
        0  // Invalid
    };

    assert(isValidMove(move));
    const PieceType captured = this->getCaptured(move);
    const PieceType promotion = movePromotionType(move);

    int score = -threshold;
    score += PIECE_VALS[captured];

    if (promotion) score += PIECE_VALS[promotion] - PIECE_VALS[PAWN];

    if (score < 0) return false;

    PieceType next = promotion ? promotion : typeOf(this->getPieceAt(moveFrom(move)));
    score -= PIECE_VALS[next];

    if (score >= 0) return true;


    const Square from   = moveFrom(move);
    const Square square = moveTo(move);

    // Sliding pieces
    const Bitboard bq = getPiecesBB(BISHOP, QUEEN);
    const Bitboard rq = getPiecesBB(ROOK,   QUEEN);

    // Occupancy and attack bitboards
    Bitboard occ = this->getPiecesBB() ^ sqToBB(from) ^ sqToBB(square);
    Bitboard atk = this->getAttackersTo(square, occ);

    // Start with opponent
    Color us = ~this->getSideToMove();

    // Pop the least valuable piece, used in loop
    const auto popLVPiece = [&] (const Bitboard atk) constexpr -> PieceType {
        for (PieceType pt = PAWN; pt <= KING; ++pt) {
            const Bitboard bb = atk & getPiecesBB(us, pt);
            if (bb) {
                const Square s = bitscan(bb);
                occ ^= sqToBB(s);
                return pt;
            }
        }
        return NO_PIECE_TYPE;
    };

    do {
        const Bitboard ourAtk = atk & getPiecesBB(us);

        // Stop when we have no attackers
        if (ourAtk == 0) break;

        // Get next piece in attack
        next = popLVPiece(ourAtk);

        // See if this opens up any diagonal / orthogonal lines
        if (next == PAWN || next == BISHOP || next == QUEEN)
            atk |= attacks<BISHOP>(square, occ) & bq;

        if (next == ROOK || next == QUEEN)
            atk |= attacks<ROOK>(square, occ) & rq;

        // Remove any used attackers from occ
        atk &= occ;

        // Make sure SEE score deteriorates over time
        score = -score - 1 - PIECE_VALS[next];
        us = ~us;

        // Hmmst
        if (score >= 0 && next == KING && (atk & getPiecesBB(us)))
            us = ~us;

    } while (score >= 0);

    return this->getSideToMove() != us;
}


template<Color Me>
bool Position::givesCheck(const Move m) const {
    assert(isValidMove(m));

    constexpr Color Opp   = ~Me;

    const Square ksq      = getKingSquare(Opp);
    const Bitboard kingBB = sqToBB(ksq);
    const Square from     = moveFrom(m);
    const Square to       = moveTo(m);
    const Bitboard occ    = getPiecesBB() ^ from;

    // Piece itself gives check
    if (pieceSees(typeOf(getPieceAt(from)), to, kingBB, occ)) return true;

    // See if there is a discovered check
    const Bitboard rqAttack = attacks<ROOK>(to, occ);
    if ((rqAttack & kingBB) && (rqAttack & getPiecesBB(Me, ROOK, QUEEN))) return true;

    const Bitboard bqAttack = attacks<BISHOP>(to, occ);
    if ((bqAttack & kingBB) && (bqAttack & getPiecesBB(Me, BISHOP, QUEEN))) return true;

    switch (moveTypeOf(m)) {
        case MT_NORMAL:
            // Already run all the checks for normal moves;
            // if we haven't already given check then the move does not
            // give check.
            return false;

        case MT_PROMOTION:
            // Check to see if the piece we are promoting into gives a check
            return pieceSees(movePromotionType(m), to, kingBB, occ);

        case MT_EN_PASSANT:
            {
                // Already checked for the piece itself checking king,
                // so we only need to check to see if we have a discovered
                // check through the captured pawn.
                const Square epCapture    = createSquare(fileOf(to), rankOf(from));
                const Bitboard occAfterEP = (getPiecesBB() ^ from ^ epCapture) | to;

                // See if either of our sliding pieces can see the king after
                // the en passant move has been made
                return (
                    attacks<ROOK>(ksq, occAfterEP)   & getPiecesBB(Me, ROOK, QUEEN)
                 || attacks<BISHOP>(ksq, occAfterEP) & getPiecesBB(Me, BISHOP, QUEEN)
                );
            }

        default:
            // castling
            // The king can't give check, see if the rook does
            if constexpr (Me == WHITE) {
                return attacks<ROOK>(to > from ? SQ_F1 : SQ_D1, occ) & kingBB;
            } else {
                return attacks<ROOK>(to > from ? SQ_F8 : SQ_D8, occ) & kingBB;
            }
    }
}

template bool Position::givesCheck<WHITE>(Move m) const;
template bool Position::givesCheck<BLACK>(Move m) const;

} // namespace Atom
