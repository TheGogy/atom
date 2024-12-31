#pragma once

#include "tt.h"
#include "types.h"

namespace Atom {

namespace Zobrist {

// Zobrist keys
extern Key keys[PIECE_NB][SQUARE_NB];
extern Key enpassantKeys[FILE_NB+1];
extern Key castlingKeys[CASTLING_RIGHT_NB];
extern Key sideToMoveKey;
extern Key noPawnsKey;

void init();

} // namespace Zobrist

} // namespace Atom
