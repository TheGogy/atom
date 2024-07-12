
#ifndef PERFT_H
#define PERFT_H

#include "position.h"

template<bool Div> size_t perft(Position &pos, int depth);
void perft(Position &pos, int depth);
void testFromFile(const std::string &filename);
#endif // !PERFT_H
