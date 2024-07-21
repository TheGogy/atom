
#include "bitboard.h"
#include "types.h"
#include <iostream>

namespace Atom {

Bitboard PAWN_ATTACK[COLOR_NB][SQUARE_NB];
Bitboard KNIGHT_MOVE[SQUARE_NB];
Bitboard KING_MOVE[SQUARE_NB];
PextEntry ROOK_MOVE[SQUARE_NB];
PextEntry BISHOP_MOVE[SQUARE_NB];
Bitboard BETWEEN_BB[SQUARE_NB][SQUARE_NB];

Bitboard ROOK_DATA[0x19000];
Bitboard BISHOP_DATA[0x1480];


// Prints the given bitboard to stdout.
// Used for debugging.
void print_bb(const Bitboard bb) {
    int sq;
    std::cout << std::endl;
    for (int rank = 7; rank >= 0; rank--) {
        std::cout << " " << rank + 1 << "│";
        for (int file = 0; file < 8; file++) {
            sq = rank * 8 + file;
            if (getBit(bb, sq))
                std::cout << "# ";
            else
                std::cout << "· ";
        }
        std::cout << std::endl;
    }
    std::cout << "  └───────────────" << std::endl;
    std::cout << "   a b c d e f g h" << std::endl;
    std::cout << "Bitboard: " << bb << std::endl;
}


// Returns a bitboard of all the squares that can be attacked up to a direction.
// This will go up to the occupied piece and stop there, including the occupied
// piece in the bitboard.
// Used to initialize sliding piece attacks.
template<Direction D>
inline Bitboard slidingRay(Square sq, Bitboard occupied) {
    Bitboard attacks = 0;
    Bitboard attacked_sq = sqToBB(sq);

    do {
        attacks |= (attacked_sq = shift<D>(attacked_sq));
    } while(attacked_sq & ~occupied);

    return attacks;
}


// Calculate the sliding piece attacks for initialization.
// Do not use this function for calculating attacks in game;
// use attacks<PieceType>() instead.
template <PieceType Pt> Bitboard slidingAttacks(Square sq, Bitboard occupied) {
  if constexpr (Pt == ROOK)
    return slidingRay<NORTH>(sq, occupied)
         | slidingRay<SOUTH>(sq, occupied)
         | slidingRay<EAST>(sq, occupied)
         | slidingRay<WEST>(sq, occupied);
  else
    return slidingRay<NORTH_EAST>(sq, occupied)
         | slidingRay<NORTH_WEST>(sq, occupied)
         | slidingRay<SOUTH_EAST>(sq, occupied)
         | slidingRay<SOUTH_WEST>(sq, occupied);
}


// Initializes the PEXT lookup table for a given sliding piece.
template<PieceType Pt>
void initPext(Square s, Bitboard table[], PextEntry magics[]) {
    static int size = 0;
    Bitboard edges, occ;

    // Define the edges of the board
    const Bitboard rankEdges = RANK_1_BB | RANK_8_BB;
    const Bitboard fileEdges = FILE_A_BB | FILE_H_BB;

    edges = (rankEdges & ~sq_to_bb(rankOf(s)))
          | (fileEdges & ~sq_to_bb(fileOf(s)));

    PextEntry& magicEntry = magics[s];
    magicEntry.mask = slidingAttacks<Pt>(s, 0) & ~edges;
    magicEntry.data = (s == SQ_A1) ? table : magics[s - 1].data + size;

    size = 0;
    occ = 0;
    do {
        magicEntry.data[pext(occ, magicEntry.mask)] = slidingAttacks<Pt>(s, occ);

        size++;
        occ = (occ - magicEntry.mask) & magicEntry.mask;
    } while (occ);
}


// Initializes the pawn attack lookups.
inline void initPawnAttacks(Square s, Bitboard bb) {
    PAWN_ATTACK[WHITE][s] = shift<NORTH_WEST>(bb) | shift<NORTH_EAST>(bb);
    PAWN_ATTACK[BLACK][s] = shift<SOUTH_EAST>(bb) | shift<SOUTH_WEST>(bb);
}


// Initializes the knight move lookups.
inline void initKnightMoves(Square s, Bitboard bb) {
    KNIGHT_MOVE[s] = shift<NORTH_WEST>(shift<NORTH>(bb)) | shift<NORTH_EAST>(shift<NORTH>(bb))
                   | shift<NORTH_EAST>(shift<EAST>(bb))  | shift<SOUTH_EAST>(shift<EAST>(bb))
                   | shift<SOUTH_EAST>(shift<SOUTH>(bb)) | shift<SOUTH_WEST>(shift<SOUTH>(bb))
                   | shift<SOUTH_WEST>(shift<WEST>(bb))  | shift<NORTH_WEST>(shift<WEST>(bb));
}


// Initializes the king move lookups.
inline void initKingMoves(Square s, Bitboard bb) {
    KING_MOVE[s] = shift<NORTH>(bb) | shift<SOUTH>(bb) | shift<EAST>(bb) | shift<WEST>(bb)
                 | shift<NORTH_EAST>(bb) | shift<NORTH_WEST>(bb) | shift<SOUTH_EAST>(bb) | shift<SOUTH_WEST>(bb);
}


// Initializes the "BETWEEN_BB" table.
// This table contains bitboards for all the squares between the two
// squares given (exclusive). For example, for squares E3 and E8:
// 0 0 0 0 0 0 0 0
// 0 0 0 0 0 0 0 0
// 0 0 0 0 0 0 0 0
// 0 0 0 1 1 1 1 0
// 0 0 0 0 0 0 0 0
// 0 0 0 0 0 0 0 0
// 0 0 0 0 0 0 0 0
// 0 0 0 0 0 0 0 0
inline void initBetweenBB(Square x, Bitboard bb_x) {
    Bitboard bb_y;
    for (Square y = SQ_ZERO; y < SQUARE_NB; ++y) {
        bb_y = sqToBB(y);
        if (slidingAttacks<ROOK>(x, EMPTY) & bb_y) {
            BETWEEN_BB[x][y] |= slidingAttacks<ROOK>(x, bb_y) & slidingAttacks<ROOK>(y, bb_x);
        } else if (slidingAttacks<BISHOP>(x, EMPTY) & bb_y) {
            BETWEEN_BB[x][y] |= slidingAttacks<BISHOP>(x, bb_y) & slidingAttacks<BISHOP>(y, bb_x);
        } 
    }
}


// Initializes all lookups. This should be called as early as possible
// when starting the program.
void initBBs() {
    for (Square s = SQ_ZERO; s < SQUARE_NB; ++s) {
        Bitboard bb = sqToBB(s);
        initPawnAttacks(s, bb);
        initKnightMoves(s, bb);
        initKingMoves(s, bb);
        initBetweenBB(s, bb);
        initPext<ROOK>(s, ROOK_DATA, ROOK_MOVE);
        initPext<BISHOP>(s, BISHOP_DATA, BISHOP_MOVE);
    }

}

} // namespace Atom
