#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <string>

using U64 = uint64_t;
constexpr U64 EMPTY = 0ULL;
constexpr U64 FULL  = 0xFFFFFFFFFFFFFFFFULL;

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
    N_SIDES = 2
};

enum PieceType {
    ALL_PIECES = 0,
    NO_PIECE_TYPE = 0,
    PAWN = 1, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    N_PIECETYPES = 7
};

enum Piece {
    NO_PIECE,
    W_PAWN = PAWN,     W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = PAWN + 8, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    N_PIECES = 16
};

enum Square : int {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    NO_SQUARE,

    FIRST_SQUARE = A1,
    N_SQUARES = 64
};

enum File : int {
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, 
    N_FILES
};

enum Rank : int {
    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, 
    N_RANKS
};

enum Direction : int {
    UP              =  8,
    DOWN            = -UP,
    RIGHT           =  1,
    LEFT            = -RIGHT,
    UP_RIGHT        = UP + RIGHT,
    UP_LEFT         = UP + LEFT,
    DOWN_RIGHT      = DOWN + RIGHT,
    DOWN_LEFT       = DOWN + LEFT,
    N_DIRS
};

constexpr int dir8(Direction d) {
    switch (d) {
        case UP:         return 0;
        case DOWN:       return 1;
        case RIGHT:      return 2;
        case LEFT:       return 3;
        case UP_RIGHT:   return 4;
        case UP_LEFT:    return 5;
        case DOWN_RIGHT: return 6;
        case DOWN_LEFT:  return 7;
    }
    return -1;
}

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

    N_CASTLINGRIGHTS = 16
};

constexpr U64 FILE_A_BB = 0x0101010101010101ULL;
constexpr U64 FILE_B_BB = FILE_A_BB << 1;
constexpr U64 FILE_C_BB = FILE_A_BB << 2;
constexpr U64 FILE_D_BB = FILE_A_BB << 3;
constexpr U64 FILE_E_BB = FILE_A_BB << 4;
constexpr U64 FILE_F_BB = FILE_A_BB << 5;
constexpr U64 FILE_G_BB = FILE_A_BB << 6;
constexpr U64 FILE_H_BB = FILE_A_BB << 7;

constexpr U64 RANK_1_BB = 0xFFULL;
constexpr U64 RANK_2_BB = RANK_1_BB << (8 * 1);
constexpr U64 RANK_3_BB = RANK_1_BB << (8 * 2);
constexpr U64 RANK_4_BB = RANK_1_BB << (8 * 3);
constexpr U64 RANK_5_BB = RANK_1_BB << (8 * 4);
constexpr U64 RANK_6_BB = RANK_1_BB << (8 * 5);
constexpr U64 RANK_7_BB = RANK_1_BB << (8 * 6);
constexpr U64 RANK_8_BB = RANK_1_BB << (8 * 7);

constexpr U64 sq_to_bb(Square s) { return (1ULL << s); }

inline Square& operator++(Square& sq) { return sq = Square(int(sq) + 1); }
inline Square& operator--(Square& sq) { return sq = Square(int(sq) - 1); }

constexpr Square operator+(Square sq, Direction dir) { return Square(int(sq) + int(dir)); }
constexpr Square operator-(Square sq, Direction dir) { return Square(int(sq) - int(dir)); }
inline Square& operator+=(Square& sq, Direction dir) { return sq = sq + dir; }
inline Square& operator-=(Square& sq, Direction dir) { return sq = sq - dir; }

constexpr CastlingRight operator|(CastlingRight cr1, CastlingRight cr2) { return CastlingRight(int(cr1) | int(cr2)); }
constexpr CastlingRight operator&(CastlingRight cr1, CastlingRight cr2) { return CastlingRight(int(cr1) & int(cr2)); }
constexpr CastlingRight operator~(CastlingRight cr1) { return CastlingRight(~int(cr1)); }
inline CastlingRight& operator|=(CastlingRight& cr, CastlingRight other) { return cr = cr | other; }
inline CastlingRight& operator&=(CastlingRight& cr, CastlingRight other) { return cr = cr & other; }
constexpr CastlingRight operator&(Side s, CastlingRight cr) { return CastlingRight((s == WHITE ? WHITE_CASTLING : BLACK_CASTLING) & cr); }

constexpr Side operator~(Side c) { return Side(c ^ BLACK); }


constexpr Square CastlingKingTo[N_CASTLINGRIGHTS] = {
    NO_SQUARE,  // NO_CASTLING
    G1,         // WHITE_KING_SIDE = 1
    C1,         // WHITE_QUEEN_SIDE = 2
    NO_SQUARE,  // Unused
    G8,         // BLACK_KING_SIDE = 4
    NO_SQUARE,  // Unused
    NO_SQUARE,  // Unused
    NO_SQUARE,  // Unused
    C8          // BLACK_QUEEN_SIDE = 8
};

// Square positions for the rook's starting position during castling
constexpr Square CastlingRookFrom[N_CASTLINGRIGHTS] = {
    NO_SQUARE,  // NO_CASTLING
    H1,         // WHITE_KING_SIDE = 1
    A1,         // WHITE_QUEEN_SIDE = 2
    NO_SQUARE,  // Unused
    H8,         // BLACK_KING_SIDE = 4
    NO_SQUARE,  // Unused
    NO_SQUARE,  // Unused
    NO_SQUARE,  // Unused
    A8          // BLACK_QUEEN_SIDE = 8
};

// Square positions for the rook's destination during castling
constexpr Square CastlingRookTo[N_CASTLINGRIGHTS] = {
    NO_SQUARE,  // NO_CASTLING
    F1,         // WHITE_KING_SIDE = 1
    D1,         // WHITE_QUEEN_SIDE = 2
    NO_SQUARE,  // Unused
    F8,         // BLACK_KING_SIDE = 4
    NO_SQUARE,  // Unused
    NO_SQUARE,  // Unused
    NO_SQUARE,  // Unused
    D8          // BLACK_QUEEN_SIDE = 8
};

constexpr CastlingRight CastlingRightsMask[N_SQUARES] = {
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
constexpr U64 CastlingPath[N_CASTLINGRIGHTS] = {
    EMPTY,                                      // NO_CASTLING
    sq_to_bb(F1) | sq_to_bb(G1),                // WHITE_KING_SIDE
    sq_to_bb(D1) | sq_to_bb(C1) | sq_to_bb(B1), // WHITE_QUEEN_SIDE
    EMPTY,                                      // Unused entry
    sq_to_bb(F8) | sq_to_bb(G8),                // BLACK_KING_SIDE
    EMPTY, EMPTY, EMPTY,                        // Unused entries
    sq_to_bb(D8) | sq_to_bb(C8) | sq_to_bb(B8)  // BLACK_QUEEN_SIDE
};

// Squares that must not be attacked by the enemy for castling
constexpr U64 CastlingKingPath[N_CASTLINGRIGHTS] = {
    EMPTY,                                      // NO_CASTLING
    sq_to_bb(E1) | sq_to_bb(F1) | sq_to_bb(G1), // WHITE_KING_SIDE
    sq_to_bb(E1) | sq_to_bb(D1) | sq_to_bb(C1), // WHITE_QUEEN_SIDE
    EMPTY,                                      // Unused entry
    sq_to_bb(E8) | sq_to_bb(F8) | sq_to_bb(G8), // BLACK_KING_SIDE
    EMPTY, EMPTY, EMPTY,                        // Unused entries
    sq_to_bb(E8) | sq_to_bb(D8) | sq_to_bb(C8)  // BLACK_QUEEN_SIDE
};

constexpr bool isValidSq(Square s) {
    return s >= A1 && s <= H8;
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
constexpr File fileOf(Square sq) { return File(sq & 7); }
constexpr Rank rankOf(Square sq) { return Rank(sq >> 3); }

constexpr U64 sq_to_bb(Rank r) { return RANK_1_BB << (8 * r); }
constexpr U64 sq_to_bb(File f) { return FILE_A_BB << f; }

constexpr Square makeSquare(File f, Rank r) { return Square((r << 3) + f); }

constexpr Move makeMove(Square from, Square to) { return Move((from << 6) + to); }

template<MoveType Type>
constexpr Move makeMove(Square from, Square to, PieceType promotionPiece = KNIGHT) {
    return Move(Type + ((promotionPiece - KNIGHT) << 12) + (from << 6) + to); 
}

constexpr Piece makePiece(Side side, PieceType p) { return Piece((side << 3) + p); }

constexpr PieceType typeOf(Piece p)   { return PieceType(p & 7); }
constexpr Side sideOf(Piece p)        { return Side(p >> 3); }

constexpr Square moveTo(Move m)       { return Square(m & 0x3F); }
constexpr Square moveFrom(Move m)     { return Square((m >> 6) & 0x3F); }
constexpr uint16_t moveFromTo(Move m) { return m & 0xFFF; }

constexpr MoveType moveType(Move m)   { return MoveType(m & (3 << 14)); }

constexpr PieceType movePromotionType(Move m) { return PieceType(((m >> 12) & 3) + KNIGHT); }

constexpr Direction pawnDirection(Side s) { return s == WHITE ? UP : DOWN; }

inline U64 operator&(U64 b, Square s) { return b & sq_to_bb(s); }
inline U64 operator|(U64 b, Square s) { return b | sq_to_bb(s); }
inline U64 operator^(U64 b, Square s) { return b ^ sq_to_bb(s); }
inline U64 operator&(Square s, U64 b) { return b & s; }
inline U64 operator|(Square s, U64 b) { return b | s; }
inline U64 operator^(Square s, U64 b) { return b ^ s; }

inline U64& operator|=(U64& b, Square s) { return b |= sq_to_bb(s); }
inline U64& operator^=(U64& b, Square s) { return b ^= sq_to_bb(s); }
inline U64  operator|(Square s1, Square s2) { return sq_to_bb(s1) | sq_to_bb(s2); }

template <typename Tn, int N, typename SizeT = uint32_t> class vector {
    Tn data[N];
    SizeT length;
public:
    typedef Tn* iterator;
    typedef const Tn* const_iterator;

    inline iterator       start()       { return &data[0]; };
    inline const_iterator start() const { return &data[0]; };
    inline iterator       end()         { return &data[length]; };
    inline const_iterator end()   const { return &data[length]; };

    vector() noexcept: length(0) {}
    inline       Tn &operator[](SizeT i)       { return data[i]; }
    inline const Tn &operator[](SizeT i) const { return data[i]; }

    inline bool isempty()  const { return length == 0; }
    inline SizeT size()    const { return length; }
    inline SizeT maxsize() const { return N; }

    inline void clear() { length = 0; }
    inline void push(const Tn& element) { data[length++] = element; }
    inline void push(      Tn& element) { data[length++] = element; }
    inline Tn   pop() const             { return data[length--]; }
};

#endif // CHESS_H_INCLUDED
