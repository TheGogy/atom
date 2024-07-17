#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "types.h"

namespace Atom {


extern Bitboard zobrist_keys[PIECE_NB][SQUARE_NB];
extern Bitboard zobrist_enpassantKeys[FILE_NB+1];
extern Bitboard zobrist_castlingKeys[CASTLING_RIGHT_NB];
extern Bitboard zobrist_sideToMoveKey;

void init_zobrist();

} // namespace atom

#endif
