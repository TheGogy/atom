#ifndef UCI_H
#define UCI_H

#include <sstream>
#include <string>

#include "engine.h"
#include "position.h"
#include "types.h"

namespace Atom {

#define ENGINE_VERSION "0.1 pre-alpha"

class Uci {
public:
    void loop();          // Main loop

    static Square parseSquare(std::string str); // Parse square
    static Move parseMove(std::string str);     // Parse move

    static std::string formatSquare(Square sq); // Format square
    static std::string formatMove(Move m);      // Format move

    static Move toMove(const Position& pos, std::string moveStr);

    static int toCentipawns(Value v, const Position &pos);

private:
    Engine engine;

    // UCI commands
    void cmdUci();
    void cmdIsReady();
    void cmdUciNewGame();
    void cmdSetOption(std::istringstream& is);
    void cmdGo(std::istringstream& is);
    void cmdStop();
    void cmdQuit();
    void cmdPerft(std::istringstream& is);
    void cmdDebug();
};

} // namespace Atom

#endif
