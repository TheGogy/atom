
#include "bitboard.h"
#include "evaluate.h"
#include "nnue.h"
#include "nnue/network.h"
#include "nnue/nnue_accumulator.h"
#include "perft.h"
#include "position.h"
#include "zobrist.h"
#include <iostream>

using namespace Atom;

// Initializes all the lookups that the engine has.
// Should be run as early as possible.
void initEverything() {
    initBBs();
    Zobrist::init();
}

int main (int argc, char *argv[]) {
    initEverything();

    Position pos;

    NNUE::Networks nnueNets = NNUE::Networks(
        NNUE::NetworkBig({EvalFileDefaultNameBig, "None", ""}, NNUE::EmbeddedNNUEType::BIG),
        NNUE::NetworkSmall({EvalFileDefaultNameSmall, "None", ""}, NNUE::EmbeddedNNUEType::SMALL)
    );

    nnueNets.big.load("<internal>", EvalFileDefaultNameBig);
    nnueNets.small.load("<internal>", EvalFileDefaultNameSmall);

    nnueNets.big.verify(EvalFileDefaultNameBig);
    nnueNets.small.verify(EvalFileDefaultNameSmall);

    NNUE::AccumulatorCaches cacheTable = {nnueNets};

    auto [psqtSmall, positionalSmall] = nnueNets.small.evaluate(pos, &cacheTable.small);
    auto [psqtBig, positionalBig] = nnueNets.big.evaluate(pos, &cacheTable.big);

    printPosition(pos);
    std::cout << "Piece value static eval:  " << Eval::pieceValueEval(pos) << std::endl;
    std::cout << "NNUE static eval (small): " << psqtSmall << " (psqt) " << positionalSmall << " (positional)" << std::endl;
    std::cout << "NNUE static eval (big):   " << psqtBig << " (psqt) " << positionalBig << " (positional)" << std::endl;

    // testFromFile("tests/perft_medium.txt");
    //
    perft(pos, 6);

    return 0;
}
