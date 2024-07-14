
#include "bitboards.h"
#include "position.h"
#include "perft.h"

int main (int argc, char *argv[]) {

    init_bbs();
    Position pos;
    pos.setFromFEN("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");

    print_position(pos);
    // testFromFile("tests/perft_vajolet.txt");

    perft(pos, 6);

    return 0;
}
