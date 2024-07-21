
#include <cstdint>
#include "zobrist.h"
#include "types.h"

namespace Atom {

namespace Zobrist {

Bitboard keys[PIECE_NB][SQUARE_NB];
Bitboard enpassantKeys[FILE_NB+1];
Bitboard castlingKeys[CASTLING_RIGHT_NB];
Bitboard sideToMoveKey;

Bitboard rand_u64() {
    // TODO: see if there is a better seed: this was chosen at random
    static uint64_t seed = 0x4E4B705B92903BA4ull;

    uint64_t val = (seed += 0x9E3779B97F4A7C15ull);
    val = (val ^ (val >> 30)) * 0xBF58476D1CE4E5B9ull;
    val = (val ^ (val >> 27)) * 0x94D049BB133111EBull;
    return val ^ (val >> 31);
}

void init() {
    for (Piece i = W_PAWN; i < PIECE_NB; ++i) {
        for (Square j = SQ_A1; j < SQUARE_NB; ++j) {
            keys[i][j] = rand_u64();
        }
    }

    for (int i = 0; i < CASTLING_RIGHT_NB; ++i) {
        castlingKeys[i] = rand_u64();
    }

    for (File i = FILE_A; i < FILE_NB; ++i) {
        enpassantKeys[i] = rand_u64();
    }
    enpassantKeys[FILE_NB] = 0;

    sideToMoveKey = rand_u64();
}

} // namespace Zobrist

} // namespace Atom
