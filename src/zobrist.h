#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "types.h"

extern U64 zobrist_keys[N_PIECES][N_SQUARES];
extern U64 zobrist_enpassantKeys[N_FILES+1];
extern U64 zobrist_castlingKeys[N_CASTLINGRIGHTS];
extern U64 zobrist_sideToMoveKey;

void init_zobrist();

#endif
