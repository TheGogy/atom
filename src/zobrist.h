#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "types.h"

namespace Atom {

namespace Zobrist {
extern Bitboard keys[PIECE_NB][SQUARE_NB];
extern Bitboard enpassantKeys[FILE_NB+1];
extern Bitboard castlingKeys[CASTLING_RIGHT_NB];
extern Bitboard sideToMoveKey;

void init();

} // namespace Zobrist

} // namespace Atom

#endif
