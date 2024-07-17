#ifndef POSITION_H
#define POSITION_H

#include "bitboards.h"
#include "types.h"
#include "zobrist.h"
#include <sstream>

namespace Atom {


#define STARTPOS_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define KIWIPETE_FEN "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"


// Board state struct. Contains information
// about the current board state used to generate, make and
// unmake moves. Part of overall position, used in Position class.
struct BoardState {
    CastlingRight castlingRights;
    Square epSquare;
    int fiftyMoveRule;
    int halfMoves;
    Move move;

    // Captured piece. This is used to unmake moves
    Piece captured;

    // Various masks used for move generation
    Bitboard attacked;
    Bitboard checkers;
    Bitboard checkMask;
    Bitboard pinDiag;
    Bitboard pinOrtho;

    // Hash, used for transposition
    uint64_t hash;

    inline BoardState &prev()             { return *(this - 1); }
    inline const BoardState &prev() const { return *(this - 1); }
    inline BoardState &next()             { return *(this + 1); }
    inline const BoardState &next() const { return *(this + 1); }
};


class Position {
public:
    Position();                                 // Default constructor.
    Position(const Position &other);            // Copy constructor.
    Position &operator=(const Position &other); // Copy assignment operator.

    void reset();                               // Reset the position to zeros.

    bool setFromFEN(const std::string &fen);    // Sets the position to given FEN.
    std::string fen() const;                    // Returns FEN of current position.
    std::string printable() const;              // Returns printable representation of the board.

    // Make and unmake the given move
    inline void doMove(Move m)   { getSideToMove() == WHITE ? doMove<WHITE>(m) : doMove<BLACK>(m); }
    inline void undoMove(Move m) { getSideToMove() == WHITE ? undoMove<WHITE>(m) : undoMove<BLACK>(m); }
    template <Side Me> inline void doMove(Move m);
    template <Side Me> inline void undoMove(Move m);

    // Returns position metadata.
    inline Side getSideToMove() const   { return sideToMove; }
    inline int getHalfMoveClock() const { return state->fiftyMoveRule; }
    inline int getHalfMoves() const     { return state->halfMoves; }
    inline int getFullMoves() const     { return 1 + (getHalfMoves() - (sideToMove == BLACK)) / 2; }
    inline Square getEpSquare() const   { return state->epSquare; }
    inline Piece getPieceAt(Square sq) const { return pieces[sq]; }
    inline bool isEmpty(Square sq) const { return getPieceAt(sq) == NO_PIECE; }
    inline bool isEmpty(Bitboard b) const    { return !(b & getPiecesBB()); }
    inline bool canCastle(CastlingRight cr) const  { return state->castlingRights & cr; }
    inline CastlingRight getCastlingRights() const { return state->castlingRights; }

    // Returns various bitboards.
    inline Bitboard getPiecesBB() const          { return sideBB[WHITE] | sideBB[BLACK]; }
    inline Bitboard getPiecesBB(Side side) const { return sideBB[side]; }
    inline Bitboard getPiecesBB(Side side, PieceType pt) const { return piecesBB[makePiece(side, pt)]; }
    inline Bitboard getPiecesBB(Side side, PieceType pt1, PieceType pt2) const {
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
    inline Square getKingSquare(Side side) const {
        return Square(bitscan(piecesBB[side == WHITE ? W_KING : B_KING]));
    }

    // Returns the count of the number of pieces.
    inline Bitboard nPieces() const                        { return popcount(getPiecesBB(WHITE) | getPiecesBB(BLACK)); }
    inline Bitboard nPieces(PieceType pt) const            { return popcount(getPiecesBB(WHITE, pt) | getPiecesBB(BLACK, pt)); }
    inline Bitboard nPieces(Side side) const               { return popcount(getPiecesBB(side)); }
    inline Bitboard nPieces(Side side, PieceType pt) const { return popcount(getPiecesBB(side, pt)); }
    inline Bitboard nPieces(Side side, PieceType pt1, PieceType pt2) const { return popcount(getPiecesBB(side, pt1, pt2)); }

    // Returns a bitboard of all the pieces that can attack the given square
    inline Bitboard getAttackersTo(const Square s, const Bitboard occ) const;

    // Returns the bitboards used for move generation.
    inline Bitboard checkMask()  const { return state->checkMask; }
    inline Bitboard pinDiag()    const { return state->pinDiag; }
    inline Bitboard pinOrtho()   const { return state->pinOrtho; }
    inline Bitboard threatened() const { return state->attacked; }
    inline Bitboard checkers()   const { return state->checkers; }
    inline Bitboard nCheckers() const { return popcount(state->checkers); }
    inline bool inCheck()        const { return !!state->checkers; }


    // Compute / get the hash of the current position.
    uint64_t computeHash() const;
    inline uint64_t hash() const { return state->hash; }

    // Get the size of the current position history.
    inline size_t historySize() const { return state - history; }

    // Check for various draws.
    inline bool isRepetitionDraw()  const;
    inline bool isMaterialDraw()    const;
    inline bool isFiftyMoveDraw()   const { return state->fiftyMoveRule > 99; }
    inline bool isDraw()            const { return isMaterialDraw() || isFiftyMoveDraw() || isRepetitionDraw(); }

    // Returns the previous move.
    inline Move previousMove()    const { return state->move; }

private:
    void setCastlingRights(CastlingRight cr);

    template <Side Me, MoveType Mt> void doMove(Move m);
    template <Side Me, MoveType Mt> void undoMove(Move m);

    template <Side Me> inline void setPiece(Square sq, Piece p);
    template <Side Me> inline void unsetPiece(Square sq);
    template <Side Me> inline void movePiece(Square from, Square to);

    template <Side Me> inline void updateThreatened();
    template <Side Me> inline void updateCheckers();
    template <Side Me, bool InCheck> inline void updatePinsAndCheckMask();

    inline void updateBitboards();
    template <Side Me> inline void updateBitboards();

    Piece pieces[SQUARE_NB];         // Array of pieces on the board.
    Bitboard sideBB[SIDE_NB];        // Bitboards for each side.
    Bitboard piecesBB[PIECE_NB];     // Bitboards for each piece type.
    Side sideToMove;                 // Side to move.
    BoardState *state;               // Pointer to the current board state.
    BoardState history[MAX_HISTORY]; // Array of board states for move history.
};

std::ostream &operator<<(std::ostream &os, const Position &pos);


// Checks to see if the position is a material draw.
// Material draws must satisfy the following conditions:
// 1. There must not exist a pawn, rook or queen on the board.
// 2. There must not exist at least one bishop on each square color for either side.
// 3. There must not exist at least three knights on the board for either side.
inline bool Position::isMaterialDraw() const {
    if ((getPiecesBB(PAWN) | getPiecesBB(ROOK) | getPiecesBB(QUEEN)) ||
       (((getPiecesBB(WHITE, BISHOP) & LIGHT_SQUARES) && (getPiecesBB(WHITE, BISHOP) & DARK_SQUARES))  ||
        ((getPiecesBB(BLACK, BISHOP) & LIGHT_SQUARES) && (getPiecesBB(BLACK, BISHOP) & DARK_SQUARES))) ||
       ((popcount(getPiecesBB(WHITE, KNIGHT)) < 3) || popcount(getPiecesBB(BLACK, KNIGHT)))) {
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
template <Side Me>
inline void Position::doMove(Move m) {
    switch (moveType(m)) {
        case NORMAL:     doMove<Me, NORMAL>(m);     return;
        case CASTLING:   doMove<Me, CASTLING>(m);   return;
        case PROMOTION:  doMove<Me, PROMOTION>(m);  return;
        case EN_PASSANT: doMove<Me, EN_PASSANT>(m); return;
    }
}

// Undoes the given move for the current position
template <Side Me>
inline void Position::undoMove(Move m) {
    switch (moveType(m)) {
        case NORMAL:     undoMove<Me, NORMAL>(m);     return;
        case CASTLING:   undoMove<Me, CASTLING>(m);   return;
        case PROMOTION:  undoMove<Me, PROMOTION>(m);  return;
        case EN_PASSANT: undoMove<Me, EN_PASSANT>(m); return;
    }
}

void print_position(const Position &pos);

} // namespace Atom

#endif
