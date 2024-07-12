
#include <cstdint>
#include "zobrist.h"

U64 zobrist_keys[N_PIECES][N_SQUARES];
U64 zobrist_enpassantKeys[N_FILES+1];
U64 zobrist_castlingKeys[N_CASTLINGRIGHTS];
U64 zobrist_sideToMoveKey;

U64 rand_u64() {
    // TODO: see if there is a better seed: this was chosen at random
    static U64 seed = 0x4E4B705B92903BA4ull;

    uint64_t val = (seed += 0x9E3779B97F4A7C15ull);
    val = (val ^ (val >> 30)) * 0xBF58476D1CE4E5B9ull;
    val = (val ^ (val >> 27)) * 0x94D049BB133111EBull;
    return val ^ (val >> 31);
}

void init_zobrist() {
    for (int i=0; i<N_PIECES; i++) {
        for (int j=0; j<N_SQUARES; j++) {
            zobrist_keys[i][j] = rand_u64();
        }
    }

    for (int i=0; i<N_CASTLINGRIGHTS; i++) {
        zobrist_castlingKeys[i] = rand_u64();
    }

    for (int i=0; i<N_FILES; i++) {
        zobrist_enpassantKeys[i] = rand_u64();
    }
    zobrist_enpassantKeys[N_FILES] = 0;

    zobrist_sideToMoveKey = rand_u64();
}
