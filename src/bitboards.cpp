
#include "bitboards.h"
#include "types.h"
#include <iostream>

U64 PAWN_ATTACK[N_SIDES][N_SQUARES];
U64 KNIGHT_MOVE[N_SQUARES];
U64 KING_MOVE[N_SQUARES];
PextEntry ROOK_MOVE[N_SQUARES];
PextEntry BISHOP_MOVE[N_SQUARES];
U64 BETWEEN_BB[N_SQUARES][N_SQUARES];

U64 ROOK_DATA[0x19000];
U64 BISHOP_DATA[0x1480];

void print_bb(const U64 bb) {
    int sq;
    std::cout << std::endl;
    for (int rank = 7; rank >= 0; rank--) {
        std::cout << " " << rank + 1 << "│";
        for (int file = 0; file < 8; file++) {
            sq = rank * 8 + file;
            if (get_bit(bb, sq))
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

template<Direction D>
inline U64 slidingRay(Square sq, U64 occupied) {
    U64 attacks = 0;
    U64 attacked_sq = sq_to_bb(sq);

    do {
        attacks |= (attacked_sq = shift<D>(attacked_sq));
    } while(attacked_sq & ~occupied);

    return attacks;
}

template <PieceType Pt> U64 slidingAttacks(Square sq, U64 occupied) {
  if constexpr (Pt == ROOK)
    return slidingRay<UP>(sq, occupied)
         | slidingRay<DOWN>(sq, occupied)
         | slidingRay<RIGHT>(sq, occupied)
         | slidingRay<LEFT>(sq, occupied);
  else
    return slidingRay<UP_RIGHT>(sq, occupied)
         | slidingRay<UP_LEFT>(sq, occupied)
         | slidingRay<DOWN_RIGHT>(sq, occupied)
         | slidingRay<DOWN_LEFT>(sq, occupied);
}

template<PieceType Pt>
void init_pext(Square s, U64 table[], PextEntry magics[]) {
    static int size = 0;
    U64 edges, occ;

    // Define the edges of the board
    const U64 rankEdges = RANK_1_BB | RANK_8_BB;
    const U64 fileEdges = FILE_A_BB | FILE_H_BB;

    edges = (rankEdges & ~sq_to_bb(rankOf(s)))
                | (fileEdges & ~sq_to_bb(fileOf(s)));

    PextEntry& magicEntry = magics[s];
    magicEntry.mask = slidingAttacks<Pt>(s, 0) & ~edges;
    magicEntry.data = (s == A1) ? table : magics[s - 1].data + size;

    size = 0;
    occ = 0;
    do {
        magicEntry.data[pext(occ, magicEntry.mask)] = slidingAttacks<Pt>(s, occ);

        size++;
        occ = (occ - magicEntry.mask) & magicEntry.mask;
    } while (occ);
}

inline void init_pawn_attacks(Square s, U64 bb) {
    PAWN_ATTACK[WHITE][s] = shift<UP_LEFT>(bb)    | shift<UP_RIGHT>(bb);
    PAWN_ATTACK[BLACK][s] = shift<DOWN_RIGHT>(bb) | shift<DOWN_LEFT>(bb);
}

inline void init_knight_move(Square s, U64 bb) {
    KNIGHT_MOVE[s] = shift<UP_LEFT>(shift<UP>(bb))      | shift<UP_RIGHT>(shift<UP>(bb))
                   | shift<UP_RIGHT>(shift<RIGHT>(bb))  | shift<DOWN_RIGHT>(shift<RIGHT>(bb))
                   | shift<DOWN_RIGHT>(shift<DOWN>(bb)) | shift<DOWN_LEFT>(shift<DOWN>(bb))
                   | shift<DOWN_LEFT>(shift<LEFT>(bb))  | shift<UP_LEFT>(shift<LEFT>(bb));
}

inline void init_king_move(Square s, U64 bb) {
    KING_MOVE[s] = shift<UP>(bb) | shift<DOWN>(bb) | shift<RIGHT>(bb) | shift<LEFT>(bb)
                 | shift<UP_RIGHT>(bb) | shift<UP_LEFT>(bb) | shift<DOWN_RIGHT>(bb) | shift<DOWN_LEFT>(bb);
}

inline void init_between_bb(Square x, U64 bb_x) {
    U64 bb_y;
    for (Square y = FIRST_SQUARE; y < N_SQUARES; ++y) {
        bb_y = sq_to_bb(y);
        if (slidingAttacks<ROOK>(x, EMPTY) & bb_y) {
            BETWEEN_BB[x][y] |= slidingAttacks<ROOK>(x, bb_y) & slidingAttacks<ROOK>(y, bb_x);
        } else if (slidingAttacks<BISHOP>(x, EMPTY) & bb_y) {
            BETWEEN_BB[x][y] |= slidingAttacks<BISHOP>(x, bb_y) & slidingAttacks<BISHOP>(y, bb_x);
        } 
    }
}

void init_bbs() {
    for (Square s = FIRST_SQUARE; s < N_SQUARES; ++s) {
        U64 bb = sq_to_bb(s);
        init_pawn_attacks(s, bb);
        init_knight_move(s, bb);
        init_king_move(s, bb);
        init_between_bb(s, bb);
        init_pext<ROOK>(s, ROOK_DATA, ROOK_MOVE);
        init_pext<BISHOP>(s, BISHOP_DATA, BISHOP_MOVE);
    }

}
