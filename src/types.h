#ifndef TYPES_H
#define TYPES_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <ostream>
#include <string>

namespace Atom {

using Bitboard = uint64_t;
constexpr Bitboard EMPTY = 0ULL;
constexpr Bitboard FULL  = 0xFFFFFFFFFFFFFFFFULL;


constexpr int MAX_PLY = 128;
constexpr int MAX_HISTORY = 2048;
constexpr int MAX_MOVE = 220;

//
// Squares
//

// clang-format off
enum Square : uint8_t {
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
// clang-format on


constexpr bool isValidSq(Square s) {
    return s >= SQ_A1 && s <= SQ_H8;
}

// Returns a bitboard with the given square set.
constexpr Bitboard sqToBB(Square s) {
    return (1ULL << s);
}


// Check if bitboard has only one bit set
constexpr bool hasOneBit(Bitboard b) {
    return __builtin_popcountll(b) == 1;
}


//
// Files and ranks
//

enum File : uint8_t {
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H,
    FILE_NB
};

enum Rank : uint8_t {
    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8,
    RANK_NB
};

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

// Create bitboard from rank / file
constexpr Bitboard fileBB(File f) { return FILE_A_BB << f; }
constexpr Bitboard rankBB(Rank r) { return RANK_1_BB << (8 * r); }

constexpr Bitboard LIGHT_SQUARES_BB = 0xAAAAAAAAAAAAAAAAULL;
constexpr Bitboard DARK_SQUARES_BB  = 0x5555555555555555ULL;

// Get rank and file information
constexpr File fileOf(Square sq) { return File(sq & 7); }
constexpr Rank rankOf(Square sq) { return Rank(sq >> 3); }

// Creates a square from the given rank and file ((r * 8) + f)
constexpr Square createSquare(File f, Rank r) { return Square((r << 3) + f); }


// Overloading some operations for squares and bitboards
inline Bitboard operator&   (Bitboard b, Square s)  { return b & sqToBB(s); }
inline Bitboard operator&   (Square s, Bitboard b)  { return b & sqToBB(s); }
inline Bitboard operator|   (Bitboard b, Square s)  { return b | sqToBB(s); }
inline Bitboard operator|   (Square s, Bitboard b)  { return b | sqToBB(s); }
inline Bitboard operator^   (Bitboard b, Square s)  { return b ^ sqToBB(s); }
inline Bitboard operator^   (Square s, Bitboard b)  { return b ^ sqToBB(s); }

inline Bitboard& operator&= (Bitboard& b, Square s) { return b &= sqToBB(s); }
inline Bitboard& operator|= (Bitboard& b, Square s) { return b |= sqToBB(s); }
inline Bitboard& operator^= (Bitboard& b, Square s) { return b ^= sqToBB(s); }
inline Bitboard  operator|  (Square s1, Square s2)  { return sqToBB(s1) | sqToBB(s2); }


//
// Color (side to play)
//

enum Color {
    WHITE, BLACK,
    COLOR_NB = 2
};

// Toggle between colors
constexpr Color operator~(Color c) { return Color(c ^ BLACK); }


//
// Pieces and PieceTypes
//

// clang-format off
enum PieceType {
    NO_PIECE_TYPE = 0,
    PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    ALL_PIECES = 0,
    PIECE_TYPE_NB = 8
};

// Check if a piece type is valid
constexpr bool isValidPieceType(PieceType pt) {
    return pt >= PAWN && pt < PIECE_TYPE_NB;
}


enum Piece {
    NO_PIECE,
    W_PAWN = PAWN,     W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = PAWN + 8, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    PIECE_NB = 16
};

// Check if a piece is valid
constexpr bool isValidPiece(Piece p) {
    switch(p) {
        case W_PAWN: case W_KNIGHT: case W_BISHOP: case W_ROOK: case W_QUEEN: case W_KING:
        case B_PAWN: case B_KNIGHT: case B_BISHOP: case B_ROOK: case B_QUEEN: case B_KING:
            return true;
        default:
            return false;
    }
}
// clang-format on

// Get piece information
constexpr PieceType typeOf(Piece p) {
    return PieceType(p & 7);
}

constexpr Color colorOf(Piece p) {
    return Color(p >> 3);
}

// Creates a piece given the side and piecetype.
constexpr Piece makePiece(Color side, PieceType p) {
    return Piece((side << 3) + p);
}


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


//
// Moves
//
// Moves are stored as 16 bit integers.
//
// Bits  0 - 5  : Destination square
// Bits  6 - 11 : Origin square
// Bits  12 - 13: Promotion piece type (0 = KNIGHT, 1 = BISHOP, 2 = ROOK, 3 = QUEEN)
// Bits  14 - 15: Move flag (0 = NORMAL, 1 = PROMOTION, 2 = EN_PASSANT, 3 = CASTLING)
//
// MOVE_NONE = empty move
// MOVE_NULL = invalid move

enum MoveType {
    MT_NORMAL,
    MT_PROMOTION  = 1 << 14,
    MT_EN_PASSANT = 2 << 14,
    MT_CASTLING   = 3 << 14
};

enum Move : uint16_t {
    MOVE_NONE,
    MOVE_NULL = 65
};


// Get move information
constexpr Square moveTo(Move m)       { return Square(m & 0x3F); }
constexpr Square moveFrom(Move m)     { return Square((m >> 6) & 0x3F); }
constexpr uint16_t moveFromTo(Move m) { return m & 0xFFF; }
constexpr MoveType moveTypeOf(Move m) { return MoveType(m & (3 << 14)); }
constexpr PieceType movePromotionType(Move m) { return PieceType(((m >> 12) & 3) + KNIGHT); }


constexpr bool isNotNullMove(Move m) {
    return (m != MOVE_NONE) && (m != MOVE_NULL);
}


constexpr bool isValidMove(Move m) {
    return isNotNullMove(m) 
        && isValidSq(moveTo(m)) 
        && isValidSq(moveFrom(m))
        && (moveTypeOf(m) == MT_PROMOTION ? isValidPieceType(movePromotionType(m)) : true);
}

// Creates a move based on the from and to squares (standard moves)
constexpr Move makeMove(Square from, Square to) { return Move((from << 6) + to); }

// Creates a move based on the from and to squares
// (special moves: EN_PASSANT, CASTLING, PROMOTION)
template<MoveType Type>
constexpr Move makeMove(Square from, Square to, PieceType promotionPiece = KNIGHT) {
    return Move(Type + ((promotionPiece - KNIGHT) << 12) + (from << 6) + to); 
}


//
// Direction
//

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

// Operations on squares with directions
constexpr Square operator+(Square sq, Direction dir) { return Square(int(sq) + int(dir)); }
constexpr Square operator-(Square sq, Direction dir) { return Square(int(sq) - int(dir)); }
inline Square& operator+=(Square& sq, Direction dir) { return sq = sq + dir; }
inline Square& operator-=(Square& sq, Direction dir) { return sq = sq - dir; }

// Find the relative pawn direction for each side
constexpr Direction pawnDirection(Color c) { return c == WHITE ? NORTH : SOUTH; }


//
// Castling rights
//

enum CastlingRight {
    NO_CASTLING      = 0,
    WHITE_OO  = 1,
    WHITE_OOO = WHITE_OO << 1,
    BLACK_OO  = WHITE_OO << 2,
    BLACK_OOO = WHITE_OO << 3,

    KING_SIDE      = WHITE_OO  | BLACK_OO,
    QUEEN_SIDE     = WHITE_OOO | BLACK_OOO,
    WHITE_CASTLING = WHITE_OO  | WHITE_OOO,
    BLACK_CASTLING = BLACK_OO  | BLACK_OOO,
    ALL_CASTLING   = WHITE_CASTLING   | BLACK_CASTLING,

    CASTLING_RIGHT_NB = 16
};


// Operations on castling rights
constexpr CastlingRight operator|(CastlingRight cr1, CastlingRight cr2) {
    return CastlingRight(int(cr1) | int(cr2));
}
constexpr CastlingRight operator&(CastlingRight cr1, CastlingRight cr2) {
    return CastlingRight(int(cr1) & int(cr2));
}
inline CastlingRight &operator|=(CastlingRight &cr, CastlingRight other) {
    return cr = cr | other;
}
inline CastlingRight &operator&=(CastlingRight &cr, CastlingRight other) {
    return cr = cr & other;
}
constexpr CastlingRight operator~(CastlingRight cr1) {
    return CastlingRight(~int(cr1));
}
constexpr CastlingRight operator&(Color s, CastlingRight cr) {
    return CastlingRight((s == WHITE ? WHITE_CASTLING : BLACK_CASTLING) & cr);
}

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
    WHITE_OOO, NO_CASTLING, NO_CASTLING, NO_CASTLING, WHITE_OOO|WHITE_OO, NO_CASTLING, NO_CASTLING, WHITE_OO,
    NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
    NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
    NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
    NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
    NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
    NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
    BLACK_OOO, NO_CASTLING, NO_CASTLING, NO_CASTLING, BLACK_OOO|BLACK_OO, NO_CASTLING, NO_CASTLING, BLACK_OO,
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



// Resizeable vector. Used for move list.
template <typename Tn, std::size_t MaxSize>
class ValueList {
public:
    ValueList() noexcept: size_(0) {}

    ValueList(std::initializer_list<Tn> il) {
        assert(il.size() <= MaxSize);
        std::copy(il.begin(), il.end(), data_);
        size_ = il.size();
    }

    ValueList(ValueList const &vl)            noexcept = default;
    ValueList(ValueList&&)                    noexcept = default;
    ValueList& operator=(ValueList const &vl) noexcept = default;
    ValueList& operator=(ValueList&&)         noexcept = default;

    // Get data from list itself
    Tn* begin() noexcept { return &data_[0]; }
    Tn* end()   noexcept { return &data_[size_]; }
    Tn &operator[](std::size_t i) noexcept {
        assert(i < size_);
        return data_[i];
    }

    const Tn* begin() const noexcept { return &data_[0]; }
    const Tn* end()   const noexcept { return &data_[size_]; }
    const Tn &operator[](std::size_t i) const noexcept {
        assert(i < size_);
        return data_[i];
    }

    const Tn* cbegin() const noexcept { return &data_[0]; }
    const Tn* cend()   const noexcept { return &data_[size_]; }

    const Tn &front() const noexcept { return data_[0]; }
    const Tn &back()  const noexcept { return data_[size_ - 1]; }
    Tn &front() noexcept { return data_[0]; }
    Tn &back()  noexcept { return data_[size_ - 1]; }

    // Get metadata about vector
    bool empty()          const noexcept { return size_ == 0; }
    std::size_t size()    const noexcept { return size_; }
    std::size_t maxsize() const noexcept { return MaxSize; }

    // Functions
    void clear() noexcept {
        size_ = 0;
    }
    void push_back(const Tn& element) noexcept {
        assert(size_ < MaxSize);
        data_[size_++] = element;
    }
    void push_back(Tn&& element) noexcept {
        assert(size_ < MaxSize);
        data_[size_++] = std::move(element);
    }
    void pop_back() noexcept {
        assert(size_ > 0);
        size_--;
    }
    void resize(std::size_t newSize) noexcept {
        assert(newSize <= size_);
        size_ = newSize;
    }
    bool contains(const Tn& e) const noexcept {
        return std::find(cbegin(), cend(), e) != end();
    }
    template <typename... Args>
    void emplace_back(Args&&... args) {
        assert(size_ < MaxSize);
        std::construct_at(&data_[size_++], std::forward<Args>(args)...);
    }

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
