#ifndef UCI_H
#define UCI_H

#include <sstream>
#include <string>

#include "position.h"
#include "types.h"

namespace Atom {

class Uci {
public:

    Uci();                                      // Constructor
    ~Uci() = default;                           // Destructor
    void loop(int argc, char* argv[]);          // Main loop

    static Square parseSquare(std::string str); // Parse square
    static Move parseMove(std::string str);     // Parse move

    static std::string formatSquare(Square sq); // Format square
    static std::string formatMove(Move m);      // Format move

    static int toCentipawns(Value v, const Position &pos);

private:
    bool cmdUci(std::istringstream& is);
};

} // namespace Atom

#endif
