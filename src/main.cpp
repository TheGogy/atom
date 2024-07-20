
#include "bitboard.h"
#include "perft.h"
#include "position.h"
#include "zobrist.h"
#include <iostream>

using namespace Atom;

// Initializes all the lookups that the engine has.
// Should be run as early as possible.
void init_all() {
    init_bbs();
    init_zobrist();
}

int main (int argc, char *argv[]) {
    init_all();
    Position pos;
    // pos.setFromFEN(KIWIPETE_FEN);
    //
    print_position(pos);
    // testFromFile("tests/perft_massive.txt");
    //
    perft(pos, 6);

    return 0;
}
