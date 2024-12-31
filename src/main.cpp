#include <cstdlib>

#include "bitboard.h"
#include "uci.h"
#include "zobrist.h"
#include "tunables.h"

using namespace Atom;

// Initializes all the lookups that the engine has.
// Should be run as early as possible.
void initEverything() {
    initBBs();
    Zobrist::init();
}

int main (int argc, char *argv[]) {
    // Print tunables
#ifdef ENABLE_TUNING
    if (argc > 1 && std::string(argv[1]) == "tunables") {
        Tunables::outputTunablesJson();
        return EXIT_SUCCESS;
    }
#endif

    initEverything();

    std::cout << "Atom v" << ENGINE_VERSION;
#ifdef NDEBUG
    std::cout << " (release)";
#elif defined(DEBUG)
    std::cout << " (debug)";
#elif defined(ENABLE_TUNING)
    std::cout << " (tuning)";
#endif
    std::cout << " built " << __DATE__ << " " << __TIME__ << std::endl;

    Uci uci;
    uci.loop();

    return EXIT_SUCCESS;
}
