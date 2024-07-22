#ifndef ENGINE_H
#define ENGINE_H

#include <cstdint>
#include <vector>

#include "bitboard.h"
#include "nnue/network.h"

namespace Atom {

class Engine {
public:

    Engine();       // Constructor
    ~Engine();      // Destructor

    void newGame(); // Reset everything

    void setPosition(const std::string& fen, const std::vector<std::string>& moves);

    // Debugging
    void runPerft(int depth);        // Runs perft (performance test)
    std::string getDebugInfo();      // Returns debug info for the position

    // Returns a visualization of various bitboards
    std::string visualizePinOrtho()   { return visualizeBB(pos.pinOrtho());   }
    std::string visualizePinDiag()    { return visualizeBB(pos.pinDiag());    }
    std::string visualizeCheckers()   { return visualizeBB(pos.checkers());   }
    std::string visualizeCheckmask()  { return visualizeBB(pos.checkMask());  }
    std::string visualizeThreatened() { return visualizeBB(pos.threatened()); }

    // NNUE
    void loadNetworks();                                // Loads NNUE networks
    void loadBigNetFromFile(const std::string& path);   // Loads big NNUE from file
    void loadSmallNetFromFile(const std::string& path); // Loads small NNUE from file
    void verifyNetworks();                              // Verify current NNUE networks

    void stop();    // Stops everything ready to be shut down

private:
    Position pos;
    NNUE::Networks networks;
};

} // namespace Atom

#endif // ENGINE_H
