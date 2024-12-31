#pragma once

#include <algorithm>

#include "nnue/network.h"
#include "nnue/nnue_accumulator.h"
#include "position.h"
#include "tunables.h"
#include "types.h"

namespace Atom {

namespace Eval {


// Blends the psqt and positional parts of the NNUE together to form a single value.
inline Value blendNnue(const int psqt, const int positional) {
    return (
        psqt       * Tunables::NNUE_PSQT_WEIGHT
      + positional * Tunables::NNUE_POSITIONAL_WEIGHT
    ) / 128;
}


// Calculates how much piece value each side has, and returns
// value(our pieces) - value (their pieces)
template <Color Me>
inline Value pieceValueEval(const Position& pos) {
    constexpr Color Opp = ~Me;
    return VALUE_PAWN   * (pos.nPieces(Me, PAWN)   - pos.nPieces(Opp, PAWN))
         + VALUE_KNIGHT * (pos.nPieces(Me, KNIGHT) - pos.nPieces(Opp, KNIGHT))
         + VALUE_BISHOP * (pos.nPieces(Me, BISHOP) - pos.nPieces(Opp, BISHOP))
         + VALUE_ROOK   * (pos.nPieces(Me, ROOK)   - pos.nPieces(Opp, ROOK))
         + VALUE_QUEEN  * (pos.nPieces(Me, QUEEN)  - pos.nPieces(Opp, QUEEN))
    ;
}


// Calculates the total material on the board, with both our piees and the opponent's pieces.
inline Value totalMaterial(const Position& pos, bool smallNet) {
    return (smallNet ? Tunables::PAWN_VALUE_SMALLNET : Tunables::PAWN_VALUE_BIGNET) * pos.nPieces(PAWN)
         + VALUE_KNIGHT * pos.nPieces(KNIGHT)
         + VALUE_BISHOP * pos.nPieces(BISHOP)
         + VALUE_ROOK   * pos.nPieces(ROOK)
         + VALUE_QUEEN  * pos.nPieces(QUEEN)
    ;
}


// Full evaluation function
template <Color Me>
Value evaluate(
    const Position& pos,
    const NNUE::Networks& networks,
    NNUE::AccumulatorCaches& cacheTables,
    Value optimism
) {
    // Positions where we are in check should not be evaluated: qsearch should search deeper.
    assert(!pos.checkers());

    // Get evaluation from various sources
    const Value pvEval = pieceValueEval<Me>(pos);
    bool smallNet = abs(pvEval) > Tunables::NNUE_SMALL_NET_THRESHOLD;
    auto [psqt, positional] = smallNet
                            ? networks.small.evaluate(pos, &cacheTables.small)
                            : networks.big.evaluate(pos, &cacheTables.big);

    Value nnueEval = blendNnue(psqt, positional);

    // If the position is confusing for the NNUE eval or pieceValueEval
    // (i.e. one thinks it is winning, the other thinks it is losing: they have opposite signs)
    // re-evaluate it with the big network
    if (smallNet && (pvEval * nnueEval < 0 || std::abs(nnueEval) < Tunables::NNUE_RE_EVALUATE_THRESHOLD)) {
        std::tie(psqt, positional) = networks.big.evaluate(pos, &cacheTables.big);
        nnueEval = blendNnue(psqt, positional);
        smallNet = false;
    }

    // Calculate complexity and subtract from eval
    Value complexity = std::abs(psqt - positional);
    nnueEval -= nnueEval * complexity / (smallNet ? Tunables::NNUE_COMPLEXITY_SMALL : Tunables::NNUE_COMPLEXITY_BIG);

    // Update optimism
    optimism += optimism * complexity / Tunables::OPTIMISM_DAMPING;

    // Get the total material on the board. This is used to scale the evaluation properly.
    Value material = totalMaterial(pos, smallNet);

    // Final evaluation is a blend between the piece value evaluation and the NNUE evaluation
    Value finalEval = (
        nnueEval * (material + Tunables::NNUE_BASE_EVAL)
      + optimism * (material + Tunables::OPTIMISM_BASE_EVAL)
    ) / Tunables::EVALUATION_NORMALIZER;

    // Damp evaluation when just shuffling pieces around
    finalEval -= finalEval * pos.getHalfMoveClock() / Tunables::RULE50_DAMPING;

    // Make sure we do not hit tablebase eval range
    return std::clamp(finalEval, VALUE_TB_LOSS_IN_MAX_PLY + 1, VALUE_TB_WIN_IN_MAX_PLY - 1);
}


} // namespace Eval

} // namespace Atom
