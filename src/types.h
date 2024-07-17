#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
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
    NORMAL,
    PROMOTION = 1 << 14,
    EN_PASSANT = 2 << 14,
    CASTLING  = 3 << 14
};


enum Side {
    WHITE, BLACK,
    SIDE_NB = 2
};


enum PieceType {
    ALL_PIECES = 0,
    NO_PIECE_TYPE = 0,
    PAWN = 1, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    PIECE_TYPE_NB = 7
};


enum Piece {
    NO_PIECE,
    W_PAWN = PAWN,     W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = PAWN + 8, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    PIECE_NB = 16
};


enum Square : int {
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
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


// Increment and decrement squares
inline Square& operator++(Square& sq) { return sq = Square(int(sq) + 1); }
inline Square& operator--(Square& sq) { return sq = Square(int(sq) - 1); }


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
constexpr CastlingRight operator&(Side s, CastlingRight cr) { return CastlingRight((s == WHITE ? WHITE_CASTLING : BLACK_CASTLING) & cr); }

constexpr Side operator~(Side c) { return Side(c ^ BLACK); }


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
constexpr Square moveTo(Move m)       { return Square(m & 0x3F); }
constexpr Square moveFrom(Move m)     { return Square((m >> 6) & 0x3F); }
constexpr uint16_t moveFromTo(Move m) { return m & 0xFFF; }
constexpr MoveType moveType(Move m)   { return MoveType(m & (3 << 14)); }
constexpr PieceType movePromotionType(Move m) { return PieceType(((m >> 12) & 3) + KNIGHT); }


// Creates a piece given the side and piecetype.
constexpr Piece makePiece(Side side, PieceType p) { return Piece((side << 3) + p); }


// Get piece information
constexpr PieceType typeOf(Piece p)   { return PieceType(p & 7); }
constexpr Side sideOf(Piece p)        { return Side(p >> 3); }


constexpr Direction pawnDirection(Side s) { return s == WHITE ? NORTH : SOUTH; }


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
template <typename Tn, int N, typename SizeT = uint32_t> 
class vector {
    Tn data[N];
    SizeT length;
public:
    typedef Tn* iterator;
    typedef const Tn* const_iterator;
    vector() noexcept: length(0) {}

    // Get data from start and end
    inline iterator       start()       { return &data[0]; };
    inline const_iterator start() const { return &data[0]; };
    inline iterator       end()         { return &data[length]; };
    inline const_iterator end()   const { return &data[length]; };

    // Get data from specific point
    inline       Tn &operator[](SizeT i)       { return data[i]; }
    inline const Tn &operator[](SizeT i) const { return data[i]; }

    // Get other data about vector
    inline bool isempty()  const { return length == 0; }
    inline SizeT size()    const { return length; }
    inline SizeT maxsize() const { return N; }

    // Functions
    inline void clear() { length = 0; }
    inline void push(const Tn& element) { data[length++] = element; }
    inline void push(      Tn& element) { data[length++] = element; }
    inline Tn   pop() const             { return data[length--]; }
};


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

} // namespace Atom

#endif // CHESS_H_INCLUDED
