#pragma once

#include "bitboard.h"
#include "nnue/nnue_accumulator.h"
#include "nnue/nnue_architecture.h"
#include "tt.h"
#include "types.h"
#include "zobrist.h"

#include <cstdint>
#include <sstream>

namespace Atom {


#define STARTPOS_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define KIWIPETE_FEN "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"


// Board state struct. Contains information
// about the current board state used to generate, make and
// unmake moves. Part of overall position, used in Position class.
struct BoardState {
    // Current board information
    CastlingRight castlingRights;
    Square        epSquare;
    int           fiftyMoveRule;
    int           halfMoves;

    // Previous move and captured piece, used to unmake moves
    Move  move;
    Piece captured;

    // Various masks used for move generation
    Bitboard attacked;
    Bitboard checkers;
    Bitboard checkMask;
    Bitboard pinDiag;
    Bitboard pinOrtho;

    // Hash, used for transposition table
    Key hash;

    // Pawn key, used for move picking
    Key pawnKey;

    // Used by NNUE
    DirtyPiece dirtyPiece;
    NNUE::Accumulator<NNUE::TransformedFeatureDimensionsBig>   accumulatorBig;
    NNUE::Accumulator<NNUE::TransformedFeatureDimensionsSmall> accumulatorSmall;

    BoardState *previous;
};


class Position {
public:
    Position();                                     // Default constructor.
    Position(const Position &other);                // Copy constructor.
    Position &operator=(const Position &other);     // Copy assignment operator.
    Position(Position &&other) noexcept;            // Move constructor.
    Position &operator=(Position &&other)noexcept;  // Move assignment operator.
    ~Position();                                    // Destructor

    void reset();                                   // Reset the position to zeros.

    bool setFromFEN(const std::string &fen);        // Sets the position to given FEN.
    std::string fen() const;                        // Returns FEN of current position.
    std::string printable() const;                  // Returns printable representation of the board.

    // Make and unmake the given move
    inline void doMove(Move m)   { getSideToMove() == WHITE ? doMove<WHITE>(m)   : doMove<BLACK>(m); }
    inline void undoMove(Move m) { getSideToMove() == WHITE ? undoMove<WHITE>(m) : undoMove<BLACK>(m); }
    template <Color Me> inline void doMove(Move m);
    template <Color Me> inline void undoMove(Move m);

    template <Color Me> void doNullMove(TranspositionTable& tt);
    template <Color Me> void undoNullMove();

    // Returns position metadata.
    inline Color getSideToMove()                   const { return sideToMove; }
    inline int getHalfMoveClock()                  const { return state->fiftyMoveRule; }
    inline int getHalfMoves()                      const { return state->halfMoves; }
    inline int getFullMoves()                      const { return 1 + (getHalfMoves() - (sideToMove == BLACK)) / 2; }
    inline Square getEpSquare()                    const { return state->epSquare; }
    inline Piece getPieceAt(const Square sq)       const { return pieces[sq]; }
    inline bool isEmpty(const Square sq)           const { return getPieceAt(sq) == NO_PIECE; }
    inline bool isEmpty(const Bitboard b)          const { return !(b & getPiecesBB()); }
    inline bool canCastle(const CastlingRight cr)  const { return state->castlingRights & cr; }
    inline CastlingRight getCastlingRights()       const { return state->castlingRights; }

    inline PieceType getCaptured(const Move m) const { return typeOf(getPieceAt(moveTo(m))); }

    // Returns various bitboards.
    inline Bitboard getPiecesBB() const           { return sideBB[WHITE] | sideBB[BLACK]; }
    inline Bitboard getPiecesBB(Color side) const { return sideBB[side]; }
    inline Bitboard getPiecesBB(Color side, PieceType pt) const { return piecesBB[makePiece(side, pt)]; }
    inline Bitboard getPiecesBB(Color side, PieceType pt1, PieceType pt2) const {
        return piecesBB[makePiece(side, pt1)] | piecesBB[makePiece(side, pt2)];
    }
    inline Bitboard getPiecesBB(PieceType pt) const {
        return getPiecesBB(WHITE, pt) | getPiecesBB(BLACK, pt);
    }
    inline Bitboard getPiecesBB(PieceType pt1, PieceType pt2) const {
        return getPiecesBB(WHITE, pt1, pt2) | getPiecesBB(BLACK, pt1, pt2);
    }

    // Returns a bitboard of all the empty squares.
    inline Bitboard getEmptyBB() const { return ~getPiecesBB(); }

    // Returns the current king square.
    inline Square getKingSquare(Color side) const {
        return Square(bitscan(piecesBB[side == WHITE ? W_KING : B_KING]));
    }

    // Returns the count of the number of pieces.
    inline Bitboard nPieces()                         const { return popcount(getPiecesBB(WHITE) | getPiecesBB(BLACK)); }
    inline Bitboard nPieces(PieceType pt)             const { return popcount(getPiecesBB(WHITE, pt) | getPiecesBB(BLACK, pt)); }
    inline Bitboard nPieces(Color side)               const { return popcount(getPiecesBB(side)); }
    inline Bitboard nPieces(Color side, PieceType pt) const { return popcount(getPiecesBB(side, pt)); }
    inline Bitboard nPieces(Color side, PieceType pt1, PieceType pt2) const { return popcount(getPiecesBB(side, pt1, pt2)); }

    // Returns a bitboard of all the pieces that can attack the given square
    inline Bitboard getAttackersTo(const Square s, const Bitboard occ) const;

    // Returns the bitboards used for move generation.
    inline Bitboard checkMask()  const { return state->checkMask; }
    inline Bitboard pinDiag()    const { return state->pinDiag;   }
    inline Bitboard pinOrtho()   const { return state->pinOrtho;  }
    inline Bitboard threatened() const { return state->attacked;  }
    inline Bitboard checkers()   const { return state->checkers;  }
    inline Bitboard nCheckers()  const { return popcount(state->checkers); }
    inline bool inCheck()        const { return !!state->checkers; }

    // Check if we have non-pawn material. This is used for some pruning
    template<Color Me> inline bool hasNonPawnMaterial() { return getPiecesBB(Me, PAWN, KING) != getPiecesBB(Me); }

    // Compute / get the hash of the current position.
    Key computeHash() const;
    inline Key hash() const { return state->hash; }
    inline Key hashAfter(const Move m) const;
    inline Key hashAfterNull() const { return hash() ^ Zobrist::sideToMoveKey; }

    // Get the size of the current position history.
    inline size_t historySize() const { return state - history; }

    // Compute / get the pawn key of the current position (used for move picker).
    Key computePawnKey() const;
    inline Key pawnKey() const { return state->pawnKey; }

    // Check for various draws.
    inline bool isRepetitionDraw()  const;
    inline bool isMaterialDraw()    const;
    inline bool isFiftyMoveDraw()   const { return state->fiftyMoveRule > 99; }
    inline bool isDraw()            const { return isMaterialDraw() || isFiftyMoveDraw() || isRepetitionDraw(); }

    // Get the previous move.
    inline Move previousMove() const { return state->move; }

    // Get the current board state (used for NNUE)
    inline BoardState* getState() const { return state; }

    // Add / remove pieces (used for NNUE / evaluation testing)
    inline void setPiece(Square sq, Piece p);
    inline void unsetPiece(Square sq);

    // Check if move is valid
    template <Color Me> bool isLegalMove(const Move m) const;
    template <Color Me> bool isPseudoLegalMove(const Move m) const;

    // Check if a move is a capture
    inline bool isCapture(const Move m) const {
        assert(isValidMove(m));

        // All captures have a piece on the "to" square except en passant
        return getPieceAt(moveTo(m)) != NO_PIECE || moveTypeOf(m) == MT_EN_PASSANT;
    }

    // Tactical moves are either captures, or queen promotions.
    inline bool isTactical(const Move m) const {
        assert(isValidMove(m));
        return isCapture(m) || (moveTypeOf(m) == MT_PROMOTION && movePromotionType(m) == QUEEN);
    }

    // Static exchange evaluation (SEE).
    bool see(const Move move, const int threshold) const;

    // Check to see if a piece can see another piece
    inline bool pieceSees(const PieceType pt, const Square seer, const Bitboard victim, const Bitboard occ) const {
        assert(isValidPieceType(pt));
        assert(isValidSq(seer));
        assert(hasOneBit(victim));

        switch (pt) {
            case PAWN:
                return (sideToMove == WHITE ? pawnAttacks<WHITE>(seer) : pawnAttacks<BLACK>(seer)) & victim;
            case KNIGHT:
                return attacks<KNIGHT>(seer) & victim;
            case BISHOP:
                return attacks<BISHOP>(seer, occ) & victim;
            case ROOK:
                return attacks<ROOK>(seer, occ) & victim;
            case QUEEN:
                return attacks<QUEEN>(seer, occ) & victim;
            case KING:
                return attacks<KING>(seer) & victim;
            default:
                // Should never reach this point
                assert(false);
                return false;
        }
    }

    // Check to see if a move gives check
    template <Color Me> bool givesCheck(const Move m) const;

private:
    void setCastlingRights(CastlingRight cr);

    template <Color Me, MoveType Mt> void doMove(Move m);
    template <Color Me, MoveType Mt> void undoMove(Move m);

    template <Color Me> inline void setPiece(Square sq, Piece p);
    template <Color Me> inline void unsetPiece(Square sq);
    template <Color Me> inline void movePiece(Square from, Square to);

    template <Color Me> inline void updateThreatened();
    template <Color Me> inline void updateCheckers();
    template <Color Me, bool InCheck> inline void updatePinsAndCheckMask();

    inline void updateBitboards();
    template <Color Me> inline void updateBitboards();

    template<Color Me, bool IsInCheck, bool IsCapture>
    inline bool isInMoveList(const Move m, const Piece pc) const;


    Piece pieces[SQUARE_NB];            // Array of pieces on the board.
    Bitboard sideBB[COLOR_NB];          // Bitboards for each side.
    Bitboard piecesBB[PIECE_NB];        // Bitboards for each piece type.
    Color sideToMove;                   // Color to move.
    BoardState *state;                  // Pointer to the current board state.
    BoardState *history;    // Array of board states for move history.
};


// Checks to see if the position is a material draw.
// Material draws must satisfy the following conditions:
// 1. There must not exist a pawn, rook or queen on the board.
// 2. There must not exist at least one bishop on each square color for either side.
// 3. There must not exist at least three knights on the board for either side.
inline bool Position::isMaterialDraw() const {

    if (
        // No pawns / rooks / queens
        (getPiecesBB(PAWN) | getPiecesBB(ROOK) | getPiecesBB(QUEEN)) ||

        // No bishop pairs
        (((getPiecesBB(WHITE, BISHOP) & LIGHT_SQUARES_BB) && (getPiecesBB(WHITE, BISHOP) & DARK_SQUARES_BB))  ||
         ((getPiecesBB(BLACK, BISHOP) & LIGHT_SQUARES_BB) && (getPiecesBB(BLACK, BISHOP) & DARK_SQUARES_BB))) ||

        // Less than 3 knights for either side
        ((popcount(getPiecesBB(WHITE, KNIGHT)) < 3) || popcount(getPiecesBB(BLACK, KNIGHT)) < 3))
    {
        return false;
    }
    return true;
}


// Check to see if the position is a repetition draw.
inline bool Position::isRepetitionDraw() const {
    if (getHalfMoveClock() < 4)
        return false;

    int reps = 0;
    const BoardState *historyStart = history + 2;
    const BoardState *fiftyMoveStart = state - getHalfMoveClock();
    const BoardState *start = fiftyMoveStart > historyStart ? fiftyMoveStart : historyStart;

    // Traverse the move history backwards in steps of 2 half-moves (1 full move)
    for (const BoardState *st = state - 2; st >= start; st -= 2) {

        // Increment the reps counter.
        // If there have been two repetitions, and the hashes match, the position
        // has been repeated 3 times: return true.
        if (st->hash == state->hash && ++reps == 2) {
            return true;
        }
    }

    return false;
}


// Does the given move for the current position
template <Color Me>
inline void Position::doMove(Move m) {
    switch (moveTypeOf(m)) {
        case MT_NORMAL:     doMove<Me, MT_NORMAL>(m);     return;
        case MT_CASTLING:   doMove<Me, MT_CASTLING>(m);   return;
        case MT_PROMOTION:  doMove<Me, MT_PROMOTION>(m);  return;
        case MT_EN_PASSANT: doMove<Me, MT_EN_PASSANT>(m); return;
    }
}

// Undoes the given move for the current position
template <Color Me>
inline void Position::undoMove(Move m) {
    switch (moveTypeOf(m)) {
        case MT_NORMAL:     undoMove<Me, MT_NORMAL>(m);     return;
        case MT_CASTLING:   undoMove<Me, MT_CASTLING>(m);   return;
        case MT_PROMOTION:  undoMove<Me, MT_PROMOTION>(m);  return;
        case MT_EN_PASSANT: undoMove<Me, MT_EN_PASSANT>(m); return;
    }
}


// Adds the piece to the position at the current square.
// For making / unmaking moves, use the templated version of
// this function instead, it will be much faster. This is just used
// to debug evaluation.
inline void Position::setPiece(Square sq, Piece p) {
    Bitboard b = sqToBB(sq);
    pieces[sq] = p;
    sideBB[colorOf(p)]  |= b;
    piecesBB[p] |= b;
}


// Removes the piece from the given square.
// For making / unmaking moves, use the templated version of
// this function instead, it will be much faster. This is just used
// to debug evaluation.
inline void Position::unsetPiece(Square sq) {
    Bitboard b = sqToBB(sq);
    Piece p = pieces[sq];
    pieces[sq] = NO_PIECE;
    sideBB[colorOf(p)]  &= ~b;
    piecesBB[p] &= ~b;
}


// Computes what the hash would be if a move is played
inline Key Position::hashAfter(const Move m) const {
    const Square from = moveFrom(m);
    const Square to   = moveTo(m);
    const Piece p     = getPieceAt(from);
    const Piece cap   = getPieceAt(to);
    Key h           = hash();

    h ^= Zobrist::sideToMoveKey;
    h ^= Zobrist::keys[p][from] ^ Zobrist::keys[p][to];
    if (cap != NO_PIECE) 
        h ^= Zobrist::keys[cap][to];

    return h;
}

} // namespace Atom
