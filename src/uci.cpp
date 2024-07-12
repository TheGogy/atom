
#include "uci.h"

std::string Uci::formatSquare(Square sq) {
    std::string str;
    str += 'a' + fileOf(sq);
    str += '1' + rankOf(sq);
    return str;
}

Square Uci::parseSquare(std::string str) {
    if (str.length() < 2) return NO_SQUARE;

    char col = str[0], row = str[1];
    if ( col < 'a' || col > 'h') return NO_SQUARE;
    if ( row < '1' || row > '8') return NO_SQUARE;

    return makeSquare(File(col - 'a'), Rank(row - '1'));
}

std::string Uci::formatMove(Move m) {
    if (m == MOVE_NONE) {
        return "(none)";
    } else if (m == MOVE_NULL) {
        return "0000";
    }

    std::string str = formatSquare(moveFrom(m)) + formatSquare(moveTo(m));

    if (moveType(m) == PROMOTION) {
        str += " pnbrqk"[movePromotionType(m)];
    }

    return str;
}

