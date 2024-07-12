
#include "bitboards.h"
#include "position.h"
#include "perft.h"

int main (int argc, char *argv[]) {

    init_bbs();
    Position pos;
    // pos.setFromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");

    print_position(pos);
    testFromFile("tests/perft_vajolet.txt");

    // perft(pos, 7);

    return 0;
}
