#ifndef POSITION_H
#define POSITION_H

#include "bitboards.h"
#include "types.h"
#include "zobrist.h"
#include <sstream>

#define STARTPOS_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

struct BoardState {
    CastlingRight castlingRights;
    Square epSquare;
    int fiftyMoveRule;
    int halfMoves;
    Move move;

    Piece captured;

    U64 attacked;
    U64 checkers;
    U64 checkMask;
    U64 pinDiag;
    U64 pinOrtho;

    uint64_t hash;

    inline BoardState &prev() { return *(this - 1); }
    inline const BoardState &prev() const { return *(this - 1); }

    inline BoardState &next() { return *(this + 1); }
    inline const BoardState &next() const { return *(this + 1); }
};

class Position {
public:
    Position();
    Position(const Position &other);
    Position &operator=(const Position &other);

    void reset();
    bool setFromFEN(const std::string &fen);
    std::string fen() const;
    std::string printable() const;

    inline void doMove(Move m)   { getSideToMove() == WHITE ? doMove<WHITE>(m) : doMove<BLACK>(m); }
    inline void undoMove(Move m) { getSideToMove() == WHITE ? undoMove<WHITE>(m) : undoMove<BLACK>(m); }
    template <Side Me> inline void doMove(Move m);
    template <Side Me> inline void undoMove(Move m);

    inline Side getSideToMove() const   { return sideToMove; }
    inline int getHalfMoveClock() const { return state->fiftyMoveRule; }
    inline int getHalfMoves() const     { return state->halfMoves; }
    inline int getFullMoves() const     { return 1 + (getHalfMoves() - (sideToMove == BLACK)) / 2; }
    inline Square getEpSquare() const   { return state->epSquare; }
    inline Piece getPieceAt(Square sq) const { return pieces[sq]; }
    inline bool isEmpty(Square sq) const { return getPieceAt(sq) == NO_PIECE; }
    inline bool isEmpty(U64 b) const    { return !(b & getPiecesBB()); }

    inline bool canCastle(CastlingRight cr) const  { return state->castlingRights & cr; }
    inline CastlingRight getCastlingRights() const { return state->castlingRights; }

    inline U64 getPiecesBB() const          { return sideBB[WHITE] | sideBB[BLACK]; }
    inline U64 getPiecesBB(Side side) const { return sideBB[side]; }
    inline U64 getPiecesBB(Side side, PieceType pt) const { return piecesBB[makePiece(side, pt)]; }
    inline U64 getPiecesBB(Side side, PieceType pt1, PieceType pt2) const {
        return piecesBB[makePiece(side, pt1)] | piecesBB[makePiece(side, pt2)];
    }
    inline U64 getPiecesBB(PieceType pt) const {
        return getPiecesBB(WHITE, pt) | getPiecesBB(BLACK, pt);
    }
    inline U64 getPiecesBB(PieceType pt1, PieceType pt2) const {
        return getPiecesBB(WHITE, pt1, pt2) | getPiecesBB(BLACK, pt1, pt2);
    }

    inline U64 getEmptyBB() const { return ~getPiecesBB(); }

    inline Square getKingSquare(Side side) const {
        return Square(bitscan(piecesBB[side == WHITE ? W_KING : B_KING]));
    }

    inline U64 nPieces() const                        { return popcount(getPiecesBB(WHITE) | getPiecesBB(BLACK)); }
    inline U64 nPieces(Side side) const               { return popcount(getPiecesBB(side)); }
    inline U64 nPieces(Side side, PieceType pt) const { return popcount(getPiecesBB(side, pt)); }
    inline U64 nPieces(Side side, PieceType pt1, PieceType pt2) const { return popcount(getPiecesBB(side, pt1, pt2)); }
    inline U64 nPieces(PieceType pt) const            { return popcount(getPiecesBB(WHITE, pt) | getPiecesBB(BLACK, pt)); }

    inline U64 threatened() const   { return state->attacked; }
    inline U64 checkers()   const   { return state->checkers; }
    inline U64 nbCheckers() const   { return popcount(state->checkers); }
    inline bool inCheck()   const   { return !!state->checkers; }

    inline uint64_t hash() const { return state->hash; }
    uint64_t computeHash() const;

    inline U64 checkMask() const { return state->checkMask; }
    inline U64 pinDiag()   const { return state->pinDiag; }
    inline U64 pinOrtho()  const { return state->pinOrtho; }

    inline size_t historySize() const { return state - history; }

    inline bool isRepetitionDraw() const;

    inline bool isFiftyMoveDraw() const { return state->fiftyMoveRule > 99; }
    inline bool isMaterialDraw()  const;
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

    Piece pieces[N_SQUARES];
    U64 sideBB[N_SIDES];
    U64 piecesBB[N_PIECES];

    Side sideToMove;

    BoardState *state;
    BoardState history[MAX_HISTORY];
};

std::ostream &operator<<(std::ostream &os, const Position &pos);

inline bool Position::isMaterialDraw() const {
    if (getPiecesBB(PAWN) | getPiecesBB(ROOK) | getPiecesBB(QUEEN))
        return false;

    if (popcount(getPiecesBB(WHITE)) > 1 && popcount(getPiecesBB(BLACK)) > 1)
        return false;

    if (popcount(getPiecesBB(KNIGHT) | getPiecesBB(BISHOP)) > 1)
        return false;

    if (!getPiecesBB(BISHOP))
        return false;

    if (popcount(getPiecesBB(KNIGHT)) < 3)
        return false;

    return true;
}

inline bool Position::isRepetitionDraw() const {
    if (getHalfMoveClock() < 4)
        return false;

    int reps = 0;
    const BoardState *historyStart = history + 2;
    const BoardState *fiftyMoveStart = state - getHalfMoveClock();
    const BoardState *start =
        fiftyMoveStart > historyStart ? fiftyMoveStart : historyStart;

    for (BoardState *st = state - 2; st >= start; st -= 2) {
        if (st->hash == state->hash && ++reps == 2) {
            return true;
        }
    }

    return false;
}

template <Side Me> inline void Position::doMove(Move m) {
    switch (moveType(m)) {
        case NORMAL:     doMove<Me, NORMAL>(m);     return;
        case CASTLING:   doMove<Me, CASTLING>(m);   return;
        case PROMOTION:  doMove<Me, PROMOTION>(m);  return;
        case EN_PASSANT: doMove<Me, EN_PASSANT>(m); return;
    }
}

template <Side Me> inline void Position::undoMove(Move m) {
    switch (moveType(m)) {
        case NORMAL:     undoMove<Me, NORMAL>(m);     return;
        case CASTLING:   undoMove<Me, CASTLING>(m);   return;
        case PROMOTION:  undoMove<Me, PROMOTION>(m);  return;
        case EN_PASSANT: undoMove<Me, EN_PASSANT>(m); return;
    }
}

void print_position(const Position &pos);

#endif
