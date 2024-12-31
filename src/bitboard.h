#pragma once

#include <immintrin.h>
#include <string>

#include "types.h"

namespace Atom {

// Bitboard utilities
inline bool getBit(Bitboard bb, Square s) {
    return bb & sqToBB(s);
}

inline int popcount(Bitboard bb) {
    return __builtin_popcountll(bb);
}

inline Square bitscan(Bitboard bb) {
    return Square(__builtin_ctzll(bb));
}

inline Bitboard lsbBitboard(Bitboard bb) {
    return _blsi_u64(bb);
}

inline uint64_t pext(uint64_t bb, uint64_t mask) {
    return _pext_u64(bb, mask);
}


template<typename F>
constexpr void loopOverBits(Bitboard bb, F f) {
    Square s;
    for(; bb; bb &= bb - 1) {
        s = bitscan(bb);
        f(s);
    }
}

template<typename F>
constexpr bool loopOverBitsUntil(Bitboard bb, F f) {
    Square s;
    for(; bb; bb &= bb - 1) {
        s = bitscan(bb);
        if (!f(s)) return false;
    }
    return true;
}


void initBBs();
std::string visualizeBB(const Bitboard bb);


// Shifts all bits in a bitboard in the given direction.
template<Direction D>
constexpr Bitboard shift(Bitboard b) {
    switch(D) {
        case NORTH:      return b << 8;
        case SOUTH:      return b >> 8;
        case EAST:       return (b & ~FILE_H_BB) << 1;
        case WEST:       return (b & ~FILE_A_BB) >> 1;
        case NORTH_EAST: return (b & ~FILE_H_BB) << 9;
        case NORTH_WEST: return (b & ~FILE_A_BB) << 7;
        case SOUTH_EAST: return (b & ~FILE_H_BB) >> 7;
        case SOUTH_WEST: return (b & ~FILE_A_BB) >> 9;
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
template<Color Me>
inline Bitboard pawnAttacks(Square sq) {
    return PAWN_ATTACK[Me][sq];
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
    assert(Pt != PAWN && Pt != NO_PIECE_TYPE && Pt != PIECE_TYPE_NB);
    if constexpr (Pt == KING)   return KING_MOVE[sq];
    if constexpr (Pt == KNIGHT) return KNIGHT_MOVE[sq];
    if constexpr (Pt == BISHOP) return sliderAttacks<BISHOP>(sq, occupied);
    if constexpr (Pt == ROOK)   return sliderAttacks<ROOK>(sq, occupied);
    if constexpr (Pt == QUEEN)  return sliderAttacks<BISHOP>(sq, occupied) | sliderAttacks<ROOK>(sq, occupied);
}

} // namespace Atom