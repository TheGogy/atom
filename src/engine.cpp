
#include "engine.h"
#include "nnue.h"
#include "nnue/network.h"
#include "perft.h"
#include "types.h"
#include "uci.h"

#include <sstream>

namespace Atom {

// Initializes the engine and loads internal networks.
Engine::Engine() :
    networks(
        NNUE::Networks(
        NNUE::NetworkBig({EvalFileDefaultNameBig, "None", ""}, NNUE::EmbeddedNNUEType::BIG),
        NNUE::NetworkSmall({EvalFileDefaultNameSmall, "None", ""}, NNUE::EmbeddedNNUEType::SMALL)
        )
    ) {
    loadNetworks();
    verifyNetworks();
}


// Engine destructor. Should exit all threads and free all memory.
Engine::~Engine() {

}


// Clears everything and sets a new game
void Engine::newGame() {

}


// Sets the position according to a given FEN and list of moves.
void Engine::setPosition(const std::string& fen, const std::vector<std::string>& moves) {
    pos.setFromFEN(fen);

    for (const std::string& moveStr : moves) {
        Move m = Uci::toMove(pos, moveStr);

        if (m == MOVE_NULL) break;

        pos.doMove(m);
    }
}


// Returns a string containing the debug info for the current position.
std::string Engine::getDebugInfo() {
    std::stringstream ss;

    ss << pos.printable() << std::endl << std::endl;
    ss << "FEN:            " << pos.fen() << std::endl;
    ss << "Hash:           " << pos.hash() << std::endl;
    ss << "Diagonal pin:   " << pos.pinDiag() << std::endl;
    ss << "Orthogonal pin: " << pos.pinOrtho() << std::endl;
    ss << "Checkmask:      " << pos.checkMask() << std::endl;

    return ss.str();
}


// Runs a perft test on the engine
void Engine::runPerft(int depth) {
    perft(pos, depth);
}


// Loads the internal NNUE networks
void Engine::loadNetworks() {
    networks.big.load("<internal>", EvalFileDefaultNameBig);
    networks.small.load("<internal>", EvalFileDefaultNameSmall);
}


// Verifies the internal NNUE networks have loaded correctly
void Engine::verifyNetworks() {
    networks.big.verify(EvalFileDefaultNameBig);
    networks.small.verify(EvalFileDefaultNameSmall);
}


// Loads respective networks from file
// HACK: This assumes NNUE files do not contain / or \.
void Engine::loadBigNetFromFile(const std::string& path) {
    size_t n = path.find_last_of("/\\");
    networks.big.load(path.substr(0, n), path.substr(n + 1));
}
void Engine::loadSmallNetFromFile(const std::string& path) {
    size_t n = path.find_last_of("/\\");
    networks.small.load(path.substr(0, n), path.substr(n + 1));
}


void Engine::stop() {

}


} // namespace Atom
