#ifndef UCI_H
#define UCI_H

#include <string>

#include "position.h"
#include "types.h"

namespace Atom {

class Uci {
public:
    static std::string formatSquare(Square sq);
    static Square parseSquare(std::string str);
    static std::string formatMove(Move m);

    static int toCentipawns(Value v, const Position &pos);
};

} // namespace Atom

#endif
