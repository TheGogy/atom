
#include <cstdint>
#include "zobrist.h"

namespace Atom {

Bitboard zobrist_keys[PIECE_NB][SQUARE_NB];
Bitboard zobrist_enpassantKeys[FILE_NB+1];
Bitboard zobrist_castlingKeys[CASTLING_RIGHT_NB];
Bitboard zobrist_sideToMoveKey;

Bitboard rand_u64() {
    // TODO: see if there is a better seed: this was chosen at random
    static Bitboard seed = 0x4E4B705B92903BA4ull;

    uint64_t val = (seed += 0x9E3779B97F4A7C15ull);
    val = (val ^ (val >> 30)) * 0xBF58476D1CE4E5B9ull;
    val = (val ^ (val >> 27)) * 0x94D049BB133111EBull;
    return val ^ (val >> 31);
}

void init_zobrist() {
    for (int i=0; i<PIECE_NB; i++) {
        for (int j=0; j<SQUARE_NB; j++) {
            zobrist_keys[i][j] = rand_u64();
        }
    }

    for (int i=0; i<CASTLING_RIGHT_NB; i++) {
        zobrist_castlingKeys[i] = rand_u64();
    }

    for (int i=0; i<FILE_NB; i++) {
        zobrist_enpassantKeys[i] = rand_u64();
    }
    zobrist_enpassantKeys[FILE_NB] = 0;

    zobrist_sideToMoveKey = rand_u64();
}

} // namespace Atom
