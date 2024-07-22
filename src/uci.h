#ifndef UCI_H
#define UCI_H

#include <sstream>
#include <string>
#include <string_view>

#include "engine.h"
#include "position.h"
#include "search.h"
#include "types.h"

namespace Atom {

#define ENGINE_VERSION "0.1 pre-alpha"

class Uci {
public:
    void loop();          // Main loop

    static std::string toLower(std::string s);

    // Helper functions
    static Square parseSquare(std::string str); // Parse square
    static Move parseMove(std::string str);     // Parse move
    static std::string formatSquare(Square sq); // Format square
    static std::string formatMove(Move m);      // Format move
    static Move toMove(const Position& pos, std::string moveStr); // Get Move from string

    static int toCentipawns(Value v, const Position &pos);

    // Callbacks for engine
    static void callbackBestMove(const std::string_view bestmove, const std::string_view ponder);
    static void callbackInfo(const EngineInfo info);

private:
    Engine engine;

    SearchLimits parseGoLimits(std::istringstream& is);

    // UCI commands
    void cmdUci();
    void cmdIsReady();
    void cmdUciNewGame();
    void cmdPosition(std::istringstream& is);
    void cmdSetOption(std::istringstream& is);
    void cmdGo(std::istringstream& is);
    void cmdStop();
    void cmdQuit();
    void cmdPerft(std::istringstream& is);
    void cmdDebug();
    void cmdVisualize(std::istringstream& is);
};

} // namespace Atom

#endif
