#ifndef BITBOARDS_H
#define BITBOARDS_H

#include <immintrin.h>
#include "types.h"

// Bitboard utilities
#define get_bit(bitboard, square) ((bitboard) &   (1ULL << (square)))
#define set_bit(bitboard, square) ((bitboard) |=  (1ULL << (square)))
#define pop_bit(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

#define bitloop(bb) for(; bb; bb &= bb - 1)
// This might be faster on some newer CPUs
// #define bitloop(bb) for(; bb; bb = _blsr_u64(bb))

#define popcount(bb) __builtin_popcountll(bb)
#define bitscan(bb) Square(__builtin_ctzll(bb))
#define pext(bb, mask) _pext_u64(bb, mask)

void init_bbs();
void print_bb(const U64 bb);

template<Direction D>
constexpr U64 shift(U64 b) {
    switch(D) {
        case UP:         return b << 8;
        case DOWN:       return b >> 8;
        case RIGHT:      return (b & ~FILE_H_BB) << 1;
        case LEFT:       return (b & ~FILE_A_BB) >> 1;
        case UP_RIGHT:   return (b & ~FILE_H_BB) << 9;
        case UP_LEFT:    return (b & ~FILE_A_BB) << 7;
        case DOWN_RIGHT: return (b & ~FILE_H_BB) >> 7;
        case DOWN_LEFT:  return (b & ~FILE_A_BB) >> 9;
    }
}

struct PextEntry {
    U64 mask;
    U64 *data;

    inline U64 attacks(U64 occ) const {
        return data[pext(occ, mask)];
    }
};

extern U64 PAWN_ATTACK[N_SIDES][N_SQUARES];
extern U64 KNIGHT_MOVE[N_SQUARES];
extern U64 KING_MOVE[N_SQUARES];
extern PextEntry BISHOP_MOVE[N_SQUARES];
extern PextEntry ROOK_MOVE[N_SQUARES];

extern U64 BETWEEN_BB[N_SQUARES][N_SQUARES];

template<Side Me>
inline U64 allPawnAttacks(U64 b) {
    if constexpr (Me == WHITE)
        return shift<UP_LEFT>(b) | shift<UP_RIGHT>(b);
    else
        return shift<DOWN_RIGHT>(b) | shift<DOWN_LEFT>(b);
}

inline U64 pawnAttacks(Side side, Square sq) {
    return PAWN_ATTACK[side][sq];
}

template<PieceType Pt>
inline U64 sliderAttacks(Square sq, U64 occupied) {
    if constexpr (Pt == ROOK)
        return ROOK_MOVE[sq].attacks(occupied);
    else
        return BISHOP_MOVE[sq].attacks(occupied);
}

template<PieceType Pt>
inline U64 attacks(Square sq, U64 occupied = 0) {
    if constexpr (Pt == KING) return KING_MOVE[sq];
    if constexpr (Pt == KNIGHT) return KNIGHT_MOVE[sq];
    if constexpr (Pt == BISHOP) return sliderAttacks<BISHOP>(sq, occupied);
    if constexpr (Pt == ROOK) return sliderAttacks<ROOK>(sq, occupied);
    if constexpr (Pt == QUEEN) return sliderAttacks<BISHOP>(sq, occupied) | sliderAttacks<ROOK>(sq, occupied);
}

#endif
