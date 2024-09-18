#ifndef TYPES_H
#define TYPES_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <ostream>
#include <string>

namespace Atom {

using Bitboard = uint64_t;
constexpr Bitboard EMPTY = 0ULL;
constexpr Bitboard FULL  = 0xFFFFFFFFFFFFFFFFULL;


constexpr int MAX_PLY = 128;
constexpr int MAX_HISTORY = 2048;
constexpr int MAX_MOVE = 220;


enum Move : uint16_t {
    MOVE_NONE,
    MOVE_NULL = 65
};


enum MoveType {
    MT_NORMAL,
    MT_PROMOTION  = 1 << 14,
    MT_EN_PASSANT = 2 << 14,
    MT_CASTLING   = 3 << 14
};


enum Color {
    WHITE, BLACK,
    COLOR_NB = 2
};


enum PieceType {
    ALL_PIECES = 0,
    NO_PIECE_TYPE = 0,
    PAWN = 1, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    PIECE_TYPE_NB = 8
};


enum Piece {
    NO_PIECE,
    W_PAWN = PAWN,     W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = PAWN + 8, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    PIECE_NB = 16
};


enum Square : int {
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
    SQ_NONE,

    SQ_ZERO = 0,
    SQUARE_NB = 64
};


enum File : int {
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, 
    FILE_NB
};


enum Rank : int {
    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, 
    RANK_NB
};


enum Direction : int {
    NORTH =  8,
    SOUTH = -8,
    EAST  =  1,
    WEST  = -1,
    NORTH_EAST = NORTH + EAST,
    NORTH_WEST = NORTH + WEST,
    SOUTH_EAST = SOUTH + EAST,
    SOUTH_WEST = SOUTH + WEST,
    DIRECTION_NB
};


enum CastlingRight {
    NO_CASTLING = 0,
    WHITE_KING_SIDE = 1,
    WHITE_QUEEN_SIDE = 2,
    BLACK_KING_SIDE  = 4,
    BLACK_QUEEN_SIDE = 8,

    KING_SIDE      = WHITE_KING_SIDE  | BLACK_KING_SIDE,
    QUEEN_SIDE     = WHITE_QUEEN_SIDE | BLACK_QUEEN_SIDE,
    WHITE_CASTLING = WHITE_KING_SIDE  | WHITE_QUEEN_SIDE,
    BLACK_CASTLING = BLACK_KING_SIDE  | BLACK_QUEEN_SIDE,
    ALL_CASTLING   = WHITE_CASTLING   | BLACK_CASTLING,

    CASTLING_RIGHT_NB = 16
};

// File and rank bitboards
constexpr Bitboard FILE_A_BB = 0x0101010101010101ULL;
constexpr Bitboard FILE_B_BB = FILE_A_BB << 1;
constexpr Bitboard FILE_C_BB = FILE_A_BB << 2;
constexpr Bitboard FILE_D_BB = FILE_A_BB << 3;
constexpr Bitboard FILE_E_BB = FILE_A_BB << 4;
constexpr Bitboard FILE_F_BB = FILE_A_BB << 5;
constexpr Bitboard FILE_G_BB = FILE_A_BB << 6;
constexpr Bitboard FILE_H_BB = FILE_A_BB << 7;

constexpr Bitboard RANK_1_BB = 0xFFULL;
constexpr Bitboard RANK_2_BB = RANK_1_BB << (8 * 1);
constexpr Bitboard RANK_3_BB = RANK_1_BB << (8 * 2);
constexpr Bitboard RANK_4_BB = RANK_1_BB << (8 * 3);
constexpr Bitboard RANK_5_BB = RANK_1_BB << (8 * 4);
constexpr Bitboard RANK_6_BB = RANK_1_BB << (8 * 5);
constexpr Bitboard RANK_7_BB = RANK_1_BB << (8 * 6);
constexpr Bitboard RANK_8_BB = RANK_1_BB << (8 * 7);


constexpr Bitboard LIGHT_SQUARES = 0xAAAAAAAAAAAAAAAAULL;
constexpr Bitboard DARK_SQUARES  = 0x5555555555555555ULL;


// Returns a bitboard with the given square set.
constexpr Bitboard sqToBB(Square s) { return (1ULL << s); }

// Allow for incrementing and decrementing some data types
#define INC_DEC_OPS_ON(T) \
    inline T& operator++(T& d) { return d = T(int(d) + 1); } \
    inline T& operator--(T& d) { return d = T(int(d) - 1); }

INC_DEC_OPS_ON(Piece)
INC_DEC_OPS_ON(PieceType)
INC_DEC_OPS_ON(Rank)
INC_DEC_OPS_ON(File)
INC_DEC_OPS_ON(Square)

#undef INC_DEC_OPS_ON

// Operations on squares with directions
constexpr Square operator+(Square sq, Direction dir) { return Square(int(sq) + int(dir)); }
constexpr Square operator-(Square sq, Direction dir) { return Square(int(sq) - int(dir)); }
inline Square& operator+=(Square& sq, Direction dir) { return sq = sq + dir; }
inline Square& operator-=(Square& sq, Direction dir) { return sq = sq - dir; }


// Operations on castling rights
constexpr CastlingRight operator|(CastlingRight cr1, CastlingRight cr2) { return CastlingRight(int(cr1) | int(cr2)); }
constexpr CastlingRight operator&(CastlingRight cr1, CastlingRight cr2) { return CastlingRight(int(cr1) & int(cr2)); }
constexpr CastlingRight operator~(CastlingRight cr1) { return CastlingRight(~int(cr1)); }
inline CastlingRight& operator|=(CastlingRight& cr, CastlingRight other) { return cr = cr | other; }
inline CastlingRight& operator&=(CastlingRight& cr, CastlingRight other) { return cr = cr & other; }
constexpr CastlingRight operator&(Color s, CastlingRight cr) { return CastlingRight((s == WHITE ? WHITE_CASTLING : BLACK_CASTLING) & cr); }

constexpr Color operator~(Color c) { return Color(c ^ BLACK); }


constexpr Square CastlingKingTo[CASTLING_RIGHT_NB] = {
    SQ_NONE,  // NO_CASTLING
    SQ_G1,    // WHITE_KING_SIDE
    SQ_C1,    // WHITE_QUEEN_SIDE
    SQ_NONE,  // Unused
    SQ_G8,    // BLACK_KING_SIDE
    SQ_NONE,  // Unused
    SQ_NONE,  // Unused
    SQ_NONE,  // Unused
    SQ_C8     // BLACK_QUEEN_SIDE
};

constexpr Square CastlingRookFrom[CASTLING_RIGHT_NB] = {
    SQ_NONE,  // NO_CASTLING
    SQ_H1,    // WHITE_KING_SIDE
    SQ_A1,    // WHITE_QUEEN_SIDE
    SQ_NONE,  // Unused
    SQ_H8,    // BLACK_KING_SIDE
    SQ_NONE,  // Unused
    SQ_NONE,  // Unused
    SQ_NONE,  // Unused
    SQ_A8     // BLACK_QUEEN_SIDE
};

constexpr Square CastlingRookTo[CASTLING_RIGHT_NB] = {
    SQ_NONE,  // NO_CASTLING
    SQ_F1,    // WHITE_KING_SIDE
    SQ_D1,    // WHITE_QUEEN_SIDE
    SQ_NONE,  // Unused
    SQ_F8,    // BLACK_KING_SIDE
    SQ_NONE,  // Unused
    SQ_NONE,  // Unused
    SQ_NONE,  // Unused
    SQ_D8     // BLACK_QUEEN_SIDE
};


// Mask to check if a piece on a square can castle
constexpr CastlingRight CastlingRightsMask[SQUARE_NB] = {
    WHITE_QUEEN_SIDE, NO_CASTLING, NO_CASTLING, NO_CASTLING, WHITE_QUEEN_SIDE|WHITE_KING_SIDE, NO_CASTLING, NO_CASTLING, WHITE_KING_SIDE,
    NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
    NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
    NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
    NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
    NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
    NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
    BLACK_QUEEN_SIDE, NO_CASTLING, NO_CASTLING, NO_CASTLING, BLACK_QUEEN_SIDE|BLACK_KING_SIDE, NO_CASTLING, NO_CASTLING, BLACK_KING_SIDE,
};


// Squares that need to be empty for castling
constexpr Bitboard CastlingPath[CASTLING_RIGHT_NB] = {
    EMPTY,
    sqToBB(SQ_F1) | sqToBB(SQ_G1),                    // White kingside
    sqToBB(SQ_D1) | sqToBB(SQ_C1) | sqToBB(SQ_B1), // White queenside
    EMPTY,
    sqToBB(SQ_F8) | sqToBB(SQ_G8),                    // Black kingside
    EMPTY,
    EMPTY,
    EMPTY,
    sqToBB(SQ_D8) | sqToBB(SQ_C8) | sqToBB(SQ_B8)  // black queenside
};


// Squares that must not be attacked by the enemy for castling
constexpr Bitboard CastlingKingPath[CASTLING_RIGHT_NB] = {
    EMPTY,
    sqToBB(SQ_E1) | sqToBB(SQ_F1) | sqToBB(SQ_G1), // White kingside
    sqToBB(SQ_E1) | sqToBB(SQ_D1) | sqToBB(SQ_C1), // White queenside
    EMPTY,
    sqToBB(SQ_E8) | sqToBB(SQ_F8) | sqToBB(SQ_G8), // Black kingside
    EMPTY,
    EMPTY,
    EMPTY,
    sqToBB(SQ_E8) | sqToBB(SQ_D8) | sqToBB(SQ_C8)  // Black queenside
};

enum Bound {
    BOUND_NONE,
    BOUND_UPPER,
    BOUND_LOWER,
    BOUND_EXACT = BOUND_UPPER | BOUND_LOWER
};


constexpr bool isValidSq(Square s) {
    return s >= SQ_A1 && s <= SQ_H8;
}

constexpr bool isValidPiece(Piece p) {
    switch(p) {
        case W_PAWN: case W_KNIGHT: case W_BISHOP: case W_ROOK: case W_QUEEN: case W_KING:
        case B_PAWN: case B_KNIGHT: case B_BISHOP: case B_ROOK: case B_QUEEN: case B_KING:
            return true;
        default:
            return false;
    }
}

constexpr bool isValidMove(Move m) {
    return (m != MOVE_NONE) && (m != MOVE_NULL);
}


// Get square information
constexpr File fileOf(Square sq) { return File(sq & 7); }
constexpr Rank rankOf(Square sq) { return Rank(sq >> 3); }


// Create bitboard from rank / file
constexpr Bitboard sq_to_bb(Rank r) { return RANK_1_BB << (8 * r); }
constexpr Bitboard sq_to_bb(File f) { return FILE_A_BB << f; }


// Creates a square from the given rank ((r * 8) + f)
constexpr Square createSquare(File f, Rank r) { return Square((r << 3) + f); }


// Creates a move based on the from and to squares (standard moves)
constexpr Move makeMove(Square from, Square to) { return Move((from << 6) + to); }

// Creates a move based on the from and to squares
// (special moves: EN_PASSANT, CASTLING, PROMOTION)
template<MoveType Type>
constexpr Move makeMove(Square from, Square to, PieceType promotionPiece = KNIGHT) {
    return Move(Type + ((promotionPiece - KNIGHT) << 12) + (from << 6) + to); 
}


// Get move information
// TODO: Move these into move struct itself?
constexpr Square moveTo(Move m)       { return Square(m & 0x3F); }
constexpr Square moveFrom(Move m)     { return Square((m >> 6) & 0x3F); }
constexpr uint16_t moveFromTo(Move m) { return m & 0xFFF; }
constexpr MoveType moveTypeOf(Move m) { return MoveType(m & (3 << 14)); }
constexpr PieceType movePromotionType(Move m) { return PieceType(((m >> 12) & 3) + KNIGHT); }


// Creates a piece given the side and piecetype.
constexpr Piece makePiece(Color side, PieceType p) { return Piece((side << 3) + p); }


// Get piece information
constexpr PieceType typeOf(Piece p)    { return PieceType(p & 7); }
constexpr Color colorOf(Piece p)       { return Color(p >> 3); }


constexpr Direction pawnDirection(Color c) { return c == WHITE ? NORTH : SOUTH; }


// Operations on bitboards and squares
inline Bitboard operator&   (Bitboard b, Square s)  { return b & sqToBB(s); }
inline Bitboard operator|   (Bitboard b, Square s)  { return b | sqToBB(s); }
inline Bitboard operator^   (Bitboard b, Square s)  { return b ^ sqToBB(s); }
inline Bitboard operator&   (Square s, Bitboard b)  { return b & s; }
inline Bitboard operator|   (Square s, Bitboard b)  { return b | s; }
inline Bitboard operator^   (Square s, Bitboard b)  { return b ^ s; }
inline Bitboard& operator|= (Bitboard& b, Square s) { return b |= sqToBB(s); }
inline Bitboard& operator^= (Bitboard& b, Square s) { return b ^= sqToBB(s); }
inline Bitboard  operator|  (Square s1, Square s2)  { return sqToBB(s1) | sqToBB(s2); }


// Resizeable vector. Used for move list.
template <typename Tn, std::size_t MaxSize>
class ValueList {
public:
    ValueList() noexcept: size_(0) {}

    inline ValueList(std::initializer_list<Tn> il) {
        copy(il.begin(), il.end(), begin());
        size_ = il.end() - il.begin();
    }
    inline ValueList(ValueList const &vl)            = default;
    inline ValueList(ValueList&&)                    = default;
    inline ValueList& operator=(ValueList const &vl) = default;
    inline ValueList& operator=(ValueList&&)         = default;

    // Get data from list itself
    inline Tn* begin()           { return &data_[0]; };
    inline Tn* end()             { return &data_[size_]; };
    inline Tn &operator[](int i) { return data_[i]; }
    inline const Tn* begin()           const { return &data_[0]; };
    inline const Tn* end()             const { return &data_[size_]; };
    inline const Tn &operator[](int i) const { return data_[i]; }

    inline const Tn &front() const { return data_[0]; }
    inline const Tn &back()  const { return data_[size_ - 1]; }
    inline Tn &front() { return data_[0]; }
    inline Tn &back()  { return data_[size_ - 1]; }

    // Get metadata about vector
    inline bool empty()          const { return size_ == 0; }
    inline std::size_t size()    const { return size_; }
    inline std::size_t maxsize() const { return MaxSize; }

    // Functions
    inline void clear() { size_ = 0; }
    inline void push_back(const Tn& element) { data_[size_++] = element; }
    inline void push_back(Tn&& element)      { data_[size_++] = element; }
    inline void pop_back() { size_--; }
    inline void resize(size_t newSize) { assert(newSize <= size_); size_ = newSize; }
    inline bool contains(const Tn &e) const { return std::find(begin(), end(), e) != end(); }
    inline bool contains(Tn &e)             { return std::find(begin(), end(), e) != end(); }

private:
    Tn data_[MaxSize];
    std::size_t size_ = 0;
};


using Depth = int;
using Value = int;

constexpr Depth QSEARCH_DEPTH_NORMAL = -1;
constexpr Depth QSEARCH_DEPTH_CHECKS =  0;


// Scored move (used for move picking)
struct ScoredMove {
    inline ScoredMove() {}
    inline ScoredMove(Move move, Value score) : move(move), score(score) {}
    Move  move;
    Value score;

    constexpr explicit operator bool() const { return move != MOVE_NONE; }
};

inline bool operator<(const ScoredMove& a, const ScoredMove& b) { return a.score < b.score; }

using MoveList = ValueList<Move, MAX_MOVE>;
using ScoredMoveList = ValueList<ScoredMove, MAX_MOVE>;
using PartialMoveList = ValueList<Move, 32>;

// Dirty (moved) piece: Keeps track of the most recently moved piece on the board.
// Used by NNUE.
struct DirtyPiece {

    // Number of changed pieces
    int dirty_num;

    // Maximum of 3 pieces can change per move.
    // This happens during a promotion capture, where:
    // Pawn           -> SQ_NONE
    // Captured piece -> SQ_NONE
    // Promoted piece -> capture square
    Piece piece[3];

    // From and to squares for each piece moved
    Square from[3];
    Square to[3];
};

constexpr Value VALUE_ZERO     = 0;
constexpr Value VALUE_DRAW     = 0;
constexpr Value VALUE_NONE     = 32002;
constexpr Value VALUE_INFINITE = 32001;

constexpr Value VALUE_MATE             = 32000;
constexpr Value VALUE_MATE_IN_MAX_PLY  = VALUE_MATE - MAX_PLY;
constexpr Value VALUE_MATED_IN_MAX_PLY = -VALUE_MATE_IN_MAX_PLY;

constexpr Value VALUE_TB                 = VALUE_MATE_IN_MAX_PLY - 1;
constexpr Value VALUE_TB_WIN_IN_MAX_PLY  = VALUE_TB - MAX_PLY;
constexpr Value VALUE_TB_LOSS_IN_MAX_PLY = -VALUE_TB_WIN_IN_MAX_PLY;

// In the code, we make the assumption that these values
// are such that non_pawn_material() can be used to uniquely
// identify the material on the board.
constexpr Value VALUE_PAWN   = 208;
constexpr Value VALUE_KNIGHT = 781;
constexpr Value VALUE_BISHOP = 825;
constexpr Value VALUE_ROOK   = 1276;
constexpr Value VALUE_QUEEN  = 2538;

constexpr Value PIECE_VALUE[PIECE_NB] = {
    VALUE_ZERO, VALUE_PAWN, VALUE_KNIGHT, VALUE_BISHOP, VALUE_ROOK, VALUE_QUEEN, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, VALUE_PAWN, VALUE_KNIGHT, VALUE_BISHOP, VALUE_ROOK, VALUE_QUEEN, VALUE_ZERO, VALUE_ZERO
};

} // namespace Atom

#endif // CHESS_H_INCLUDED
