#ifndef BITBOARD_H
#define BITBOARD_H

#include <immintrin.h>
#include "types.h"

namespace Atom {

// Bitboard utilities
#define getBit(bitboard, square) ((bitboard) &   (1ULL << (square)))
#define setBit(bitboard, square) ((bitboard) |=  (1ULL << (square)))
#define popBit(bitboard, square) ((bitboard) &= ~(1ULL << (square)))


#define bitloop(bb) for(; bb; bb &= bb - 1)
// This might be faster on some newer CPUs
// #define bitloop(bb) for(; bb; bb = _blsr_u64(bb))

#define popcount(bb) __builtin_popcountll(bb)
#define bitscan(bb) Square(_tzcnt_u64(bb))
#define mask(bitboard) _blsi_u64(bitboard)
#define pext(bb, mask) _pext_u64(bb, mask)


void initBBs();
void print_bb(const Bitboard bb);


// Shifts all bits in a bitboard in the given direction.
template<Direction D>
constexpr Bitboard shift(Bitboard b) {
    switch(D) {
        case NORTH:         return b << 8;
        case SOUTH:       return b >> 8;
        case EAST:      return (b & ~FILE_H_BB) << 1;
        case WEST:       return (b & ~FILE_A_BB) >> 1;
        case NORTH_EAST:   return (b & ~FILE_H_BB) << 9;
        case NORTH_WEST:    return (b & ~FILE_A_BB) << 7;
        case SOUTH_EAST: return (b & ~FILE_H_BB) >> 7;
        case SOUTH_WEST:  return (b & ~FILE_A_BB) >> 9;
    }
}


struct PextEntry {
    Bitboard mask;
    Bitboard *data;

    inline Bitboard attacks(Bitboard occ) const {
        return data[pext(occ, mask)];
    }
};


extern Bitboard PAWN_ATTACK[COLOR_NB][SQUARE_NB];
extern Bitboard KNIGHT_MOVE[SQUARE_NB];
extern Bitboard KING_MOVE[SQUARE_NB];
extern PextEntry BISHOP_MOVE[SQUARE_NB];
extern PextEntry ROOK_MOVE[SQUARE_NB];

extern Bitboard BETWEEN_BB[SQUARE_NB][SQUARE_NB];


// Returns a bitboard of all the pseudo legal pawn attacks, given the pawn bitboard.
template<Color Me>
inline Bitboard allPawnAttacks(Bitboard b) {
    if constexpr (Me == WHITE)
        return shift<NORTH_WEST>(b) | shift<NORTH_EAST>(b);
    else
        return shift<SOUTH_EAST>(b) | shift<SOUTH_WEST>(b);
}


// Returns a bitboard of the pawn attacks for a single square and color.
inline Bitboard pawnAttacks(Color c, Square sq) {
    return PAWN_ATTACK[c][sq];
}


// Returns a bitboard of all the pseudo legal moves for a sliding piece.
template<PieceType Pt>
inline Bitboard sliderAttacks(Square sq, Bitboard occupied) {
    if constexpr (Pt == ROOK)
        return ROOK_MOVE[sq].attacks(occupied);
    else
        return BISHOP_MOVE[sq].attacks(occupied);
}

// Returns a bitboard of all the pseudo legal moves for the given piece.
// For pawns, use pawnAttacks().
template<PieceType Pt>
inline Bitboard attacks(Square sq, Bitboard occupied = 0) {
    if constexpr (Pt == KING) return KING_MOVE[sq];
    if constexpr (Pt == KNIGHT) return KNIGHT_MOVE[sq];
    if constexpr (Pt == BISHOP) return sliderAttacks<BISHOP>(sq, occupied);
    if constexpr (Pt == ROOK) return sliderAttacks<ROOK>(sq, occupied);
    if constexpr (Pt == QUEEN) return sliderAttacks<BISHOP>(sq, occupied) | sliderAttacks<ROOK>(sq, occupied);
}

} // namespace Atom

#endif // BITBOARD_H
