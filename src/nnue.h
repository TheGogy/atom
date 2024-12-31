#pragma once

// From stockfish. For more info, see nnue/README.md

#include "evaluate.h"
#include "position.h"
#include "types.h"

namespace Atom {

// Edit these names here when using new NNUEs.
// Ensure that they are correct with the NNUE path from the
// directory that you are building from.
#define EvalFileDefaultNameBig   "nn-1c0000000000.nnue"
#define EvalFileDefaultNameSmall "nn-37f18f62d772.nnue"

namespace NNUE {

struct Networks;
struct AccumulatorCaches;

inline bool useSmallNet(const Position &pos) {
    const Value pvEval = pos.getSideToMove() == WHITE ? Eval::pieceValueEval<WHITE>(pos) : Eval::pieceValueEval<BLACK>(pos);
    return std::abs(pvEval) > 962;
}

} // namespace NNUE

} // namespace Atom
