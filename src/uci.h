#ifndef UCI_H
#define UCI_H

#include <string>
#include "types.h"

class Uci {
public:
    static std::string formatSquare(Square sq);
    static Square parseSquare(std::string str);
    static std::string formatMove(Move m);
};

#endif
