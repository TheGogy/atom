#ifndef ENGINE_H
#define ENGINE_H

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "bitboard.h"
#include "nnue/network.h"
#include "search.h"
#include "types.h"

namespace Atom {

struct EngineInfo {
    int depth;
    int selDepth;
    size_t timeSearched;
    size_t nodesSearched;
    std::string_view pv;
    Value score;
    int hashFull;
};


class Engine {
public:

    Engine();       // Constructor
    ~Engine();      // Destructor

    void newGame(); // Reset everything

    void setPosition(const std::string& fen, const std::vector<std::string>& moves);

    // Debugging
    void runPerft(int depth);                         // Runs perft (performance test)
    std::string getDebugInfo();                       // Returns debug info for the position
    std::string getFen() const { return pos.fen(); }  // Returns the FEN of the position

    // Returns a visualization of various bitboards
    std::string visualizePinOrtho()   { return visualizeBB(pos.pinOrtho());   }
    std::string visualizePinDiag()    { return visualizeBB(pos.pinDiag());    }
    std::string visualizeCheckers()   { return visualizeBB(pos.checkers());   }
    std::string visualizeCheckmask()  { return visualizeBB(pos.checkMask());  }
    std::string visualizeThreatened() { return visualizeBB(pos.threatened()); }

    // NNUE
    void loadNetworks();
    void loadBigNetFromFile(const std::string& path);
    void loadSmallNetFromFile(const std::string& path);
    void verifyNetworks();

    // Runs respective UCI commands
    void go(SearchLimits limits);
    void stop();
    void traceEval();

private:
    Position pos;
    NNUE::Networks networks;
};

} // namespace Atom

#endif // ENGINE_H
