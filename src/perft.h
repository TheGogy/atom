
#ifndef PERFT_H
#define PERFT_H

#include "position.h"

namespace Atom {

template <bool Div>
std::size_t perft(Position &pos, int depth);

void perft(Position &pos, int depth);
void testFromFile(const std::string &filename);

} // namespace Atom

#endif // !PERFT_H
