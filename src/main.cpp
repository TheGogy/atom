#include <cstdlib>

#include "bitboard.h"
#include "uci.h"
#include "zobrist.h"

using namespace Atom;

// Initializes all the lookups that the engine has.
// Should be run as early as possible.
void initEverything() {
    initBBs();
    Zobrist::init();
}

int main (int argc, char *argv[]) {
    initEverything();

    std::cout << "Atom v" << ENGINE_VERSION;
#ifdef RELEASE
    std::cout << " (release)";
#elif defined(DEBUG)
    std::cout << " (debug)";
#endif
    std::cout << " built " << __DATE__ << " " << __TIME__ << std::endl;

    Uci uci;
    uci.loop();

    return EXIT_SUCCESS;
}
