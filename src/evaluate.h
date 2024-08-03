#ifndef EVALUATE_H
#define EVALUATE_H

#include <algorithm>

#include "nnue/network.h"
#include "nnue/nnue_accumulator.h"
#include "position.h"
#include "tunables.h"
#include "types.h"

namespace Atom {

namespace Eval {


inline Value blendNnue(const int psqt, const int positional) {
    return (
        psqt       * Tunables::NNUE_PSQT_WEIGHT
      + positional * Tunables::NNUE_POSITIONAL_WEIGHT
    ) / 128;
}


template <Color Me>
inline int pieceValueEval(const Position& pos) {
    constexpr Color Opp = ~Me;
    return VALUE_PAWN   * (pos.nPieces(Me, PAWN)   - pos.nPieces(Opp, PAWN))
         + VALUE_KNIGHT * (pos.nPieces(Me, KNIGHT) - pos.nPieces(Opp, KNIGHT))
         + VALUE_BISHOP * (pos.nPieces(Me, BISHOP) - pos.nPieces(Opp, BISHOP))
         + VALUE_ROOK   * (pos.nPieces(Me, ROOK)   - pos.nPieces(Opp, ROOK))
         + VALUE_QUEEN  * (pos.nPieces(Me, QUEEN)  - pos.nPieces(Opp, QUEEN))
    ;
}


// Full evaluation function
template <Color Me>
Value evaluate(
    const Position& pos,
    const NNUE::Networks& networks,
    NNUE::AccumulatorCaches& cacheTables)
{
    const int pvEval = pieceValueEval<Me>(pos);
    bool smallNet = abs(pvEval) > Tunables::NNUE_SMALL_NET_THRESHOLD;
    auto [psqt, positional] = smallNet
                            ? networks.small.evaluate(pos, &cacheTables.small)
                            : networks.big.evaluate(pos, &cacheTables.big);

    Value nnueEval = blendNnue(psqt, positional);

    // If the position is confusing for the NNUE eval or pieceValueEval
    // (i.e. one thinks it is winning, the other thinks it is losing)
    // re-evaluate it with the big network
    if (smallNet && ((pvEval ^ nnueEval) < 0)) {
        std::tie(psqt, positional) = networks.big.evaluate(pos, &cacheTables.big);
        nnueEval = blendNnue(psqt, positional);
        smallNet = false;
    }

    Value finalEval = (
        nnueEval * (pvEval + Tunables::NNUE_BASE_EVAL)
    ) / (smallNet ? Tunables::EVALUATION_NORMALIZER_SMALLNET : Tunables::EVALUATION_NORMALIZER_BIGNET);

    // Make sure we do not hit tablebase eval range
    return std::clamp(finalEval, VALUE_TB_LOSS_IN_MAX_PLY + 1, VALUE_TB_WIN_IN_MAX_PLY - 1);
}


} // namespace Eval

} // namespace Atom

#endif // EVALUATE_H
