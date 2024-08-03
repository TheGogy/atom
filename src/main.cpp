#include "bitboard.h"
#include "uci.h"
#include "zobrist.h"
#include <cstddef>
#include <cstdlib>
#include <ctime>

using namespace Atom;

// Initializes all the lookups that the engine has.
// Should be run as early as possible.
void initEverything() {
    initBBs();
    Zobrist::init();
    srand(time(NULL));
}

int main (int argc, char *argv[]) {
    initEverything();

    Uci uci;
    uci.loop();

    return 0;
}
