#ifndef NNUE_H
#define NNUE_H

// From stockfish. For more info, see nnue/README.md

#include "evaluate.h"
#include "position.h"

namespace Atom {

#define EvalFileDefaultNameBig "nn-ea8c9128c325.nnue"
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
