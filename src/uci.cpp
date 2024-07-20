
#include "uci.h"
#include "position.h"
#include "types.h"
#include <algorithm>
#include <cmath>

namespace Atom {

// Formats the given square according to the UCI standard (a1 --> h8)
std::string Uci::formatSquare(Square sq) {
    std::string str;
    str += 'a' + fileOf(sq);
    str += '1' + rankOf(sq);
    return str;
}


// Parses the given square according to UCI standard (a1 --> h8)
Square Uci::parseSquare(std::string str) {
    if (str.length() < 2) return SQ_NONE;

    char col = str[0], row = str[1];
    if ( col < 'a' || col > 'h') return SQ_NONE;
    if ( row < '1' || row > '8') return SQ_NONE;

    return createSquare(File(col - 'a'), Rank(row - '1'));
}


// Formats the given move according to UCI standard.
// This is: <square from><square to><promotion piece>
// e.g "e2e4"
// or  "e7e8q"
//
// Null moves are returned as "0000", although this should
// not happen in normal play.
std::string Uci::formatMove(Move m) {
    if (m == MOVE_NONE) {
        return "(none)";
    } else if (m == MOVE_NULL) {
        return "0000";
    }

    std::string str = formatSquare(moveFrom(m)) + formatSquare(moveTo(m));

    if (moveType(m) == PROMOTION) {
        str += "?pnbrq?"[movePromotionType(m)];
    }

    return str;
}


// Gets the win rate parameters, taken from stockfish to ensure
// compatibility with NNUE output.
struct WinRateParams {
    double a;
    double b;
};

WinRateParams getWinRateParams(const Position &pos) {
    int material = pos.nPieces(PAWN) +
                   pos.nPieces(KNIGHT) * 3 +
                   pos.nPieces(BISHOP) * 3 +
                   pos.nPieces(ROOK)   * 5 +
                   pos.nPieces(QUEEN)  * 9;

    double m = std::clamp(material, 17, 78) / 58.0;
    constexpr double as[] = {-41.25712052, 121.47473115, -124.46958843, 411.84490997};
    constexpr double bs[] = {84.92998051, -143.66658718, 80.09988253, 49.80869370};

    double a = (((as[0] * m + as[1]) * m + as[2]) * m) + as[3];
    double b = (((bs[0] * m + bs[1]) * m + bs[2]) * m) + bs[3];

    return {a, b};
}


int Uci::toCentipawns(Value v, const Position &pos) {
    auto [a, b] = getWinRateParams(pos);
    return std::round(100 * int(v) / a);
}

} // namespace Atom

