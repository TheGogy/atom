
#include "bitboards.h"
#include "position.h"
#include "perft.h"

using namespace Atom;

int main (int argc, char *argv[]) {

    init_bbs();
    Position pos;
    pos.setFromFEN(KIWIPETE_FEN);

    print_position(pos);
    // testFromFile("tests/perft_small.txt");

    perft(pos, 7);

    return 0;
}
