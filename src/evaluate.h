#ifndef EVALUATE_H
#define EVALUATE_H

#include "position.h"
#include "types.h"

namespace Atom {

namespace Eval {

inline int pieceValueEval(const Position& pos) {
    Color Me = pos.getSideToMove();
    Color Opp = ~Me;
    return VALUE_PAWN   * (pos.nPieces(Me, PAWN)   - pos.nPieces(Opp, PAWN))
         + VALUE_KNIGHT * (pos.nPieces(Me, KNIGHT) - pos.nPieces(Opp, KNIGHT))
         + VALUE_BISHOP * (pos.nPieces(Me, BISHOP) - pos.nPieces(Opp, BISHOP))
         + VALUE_ROOK   * (pos.nPieces(Me, ROOK)   - pos.nPieces(Opp, ROOK))
         + VALUE_QUEEN  * (pos.nPieces(Me, QUEEN)  - pos.nPieces(Opp, QUEEN))
    ;
}

} // namespace Eval

} // namespace Atom

#endif // EVALUATE_H
