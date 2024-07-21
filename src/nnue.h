#ifndef NNUE_H
#define NNUE_H

// From stockfish. For more info, see nnue/README.md

#include "evaluate.h"
#include "position.h"

namespace Atom {

// Edit these names here when using new NNUEs.
// Ensure that they are correct with the NNUE path from the
// directory that you are building from.
#define EvalFileDefaultNameBig "nn-68207f2da9ea.nnue"
#define EvalFileDefaultNameSmall "nn-37f18f62d772.nnue"

namespace NNUE {

struct Networks;
struct AccumulatorCaches;

inline bool useSmallNet(const Position &pos) {
    return std::abs(Eval::pieceValueEval(pos)) > 962;
}

} // namespace NNUE

} // namespace Atom

#endif // NNUE_H
