#include <cstdlib>
#include <memory>
#include <sstream>
#include <vector>

#include "engine.h"
#include "evaluate.h"
#include "nnue.h"
#include "nnue/network.h"
#include "nnue/nnue_misc.h"
#include "perft.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "types.h"
#include "uci.h"

namespace Atom {

// Initializes the engine and loads internal networks.
Engine::Engine() :
    networks(
        NNUE::Networks(
        NNUE::NetworkBig({EvalFileDefaultNameBig, "None", ""}, NNUE::EmbeddedNNUEType::BIG),
        NNUE::NetworkSmall({EvalFileDefaultNameSmall, "None", ""}, NNUE::EmbeddedNNUEType::SMALL)
        )
    ) {
    loadInternalNNUEs();
    Search::SearchWorkerShared sharedState = {threads, networks, tt};
    threads.setNbThreads(NB_THREADS_DEFAULT, sharedState);
}


// Clears everything and sets a new game
void Engine::newGame() {
    pos.setFromFEN(STARTPOS_FEN);
    tt.clear();
    threads.clearThreads();
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
void Engine::loadInternalNNUEs() {
    networks.big.load("<internal>", EvalFileDefaultNameBig);
    networks.small.load("<internal>", EvalFileDefaultNameSmall);
    verifyNetworks();
}


// Verifies the internal NNUE networks have loaded correctly
void Engine::verifyNetworks() {
    networks.big.verify(EvalFileDefaultNameBig);
    networks.small.verify(EvalFileDefaultNameSmall);
}


// Loads respective networks from file
// HACK: This assumes the names of the NNUE files themselves do not contain / or \.
void Engine::loadBigNetFromFile(const std::string& path) {
    size_t n = path.find_last_of("/\\");
    networks.big.load(path.substr(0, n), path.substr(n + 1));
    networks.big.verify(EvalFileDefaultNameBig);
}
void Engine::loadSmallNetFromFile(const std::string& path) {
    size_t n = path.find_last_of("/\\");
    networks.small.load(path.substr(0, n), path.substr(n + 1));
    networks.small.verify(EvalFileDefaultNameSmall);
}

//
//  UCI engine related commands
//  For more info, see uci.cpp
//

// UCI Go command. Starts searching at the current position.
void Engine::go(Search::SearchLimits limits) {
    threads.go(pos, limits);
}


// UCI stop command. Stops searching at the current position and returns bestmove.
void Engine::stop() {
    threads.shouldStop = true;
}


// Traces the evaluation from the current position and shows the contributions
// of various evaluation sources.
void Engine::traceEval() {
    verifyNetworks();

    if (pos.inCheck()) {
        std::cout << "Warning: in check. This position will not be evaluated in normal search." << std::endl;
    }

    auto caches = std::make_unique<NNUE::AccumulatorCaches>(networks);
    std::cout << NNUE::trace(pos, networks, *caches) << std::endl;

    Value v = pos.getSideToMove() == WHITE
        ?  Eval::evaluate<WHITE>(pos, networks, *caches, 0)
        : -Eval::evaluate<BLACK>(pos, networks, *caches, 0);

    std::cout << "final evaluation: " << 0.01 * Uci::toCentipawns(v, pos) << " (white's perspective)" << std::endl;
}


void Engine::waitForSearchFinish() {
    threads.firstThread()->waitForFinish();
}


void Engine::clear() {
    waitForSearchFinish();
    tt.clear();
    threads.clearThreads();
}

} // namespace Atom
