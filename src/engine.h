#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "bitboard.h"
#include "nnue/network.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "types.h"

namespace Atom {

class Engine {
public:

    Engine();                              // Constructor
    ~Engine() { waitForSearchFinish(); }   // Destructor

    void setPosition(const std::string& fen, const std::vector<std::string>& moves);

    // Debugging
    void runPerft(int depth);
    std::string getDebugInfo();
    std::string getFen() const { return pos.fen(); }

    // Returns a visualization of various bitboards
    std::string visualizePinOrtho()   { return visualizeBB(pos.pinOrtho());   }
    std::string visualizePinDiag()    { return visualizeBB(pos.pinDiag());    }
    std::string visualizeCheckers()   { return visualizeBB(pos.checkers());   }
    std::string visualizeCheckmask()  { return visualizeBB(pos.checkMask());  }
    std::string visualizeThreatened() { return visualizeBB(pos.threatened()); }

    // NNUE
    void loadInternalNNUEs();
    void loadBigNetFromFile(const std::string& path);
    void loadSmallNetFromFile(const std::string& path);
    void verifyNetworks();

    // Runs respective UCI commands
    void go(Search::SearchLimits limits);
    void stop();
    void traceEval();
    void newGame();
    void clear();

    // Set aspects of engine
    inline void setHashSize(size_t newSize) { tt.resize(newSize); }
    inline void setNbThreads(size_t nbThreads) { threads.setNbThreads(nbThreads, {threads, networks, tt}); }

    // Search
    void waitForSearchFinish();
    inline bool isSearching() { return threads.firstThread()->isSearching(); }

private:
    Position pos;

    ThreadPool threads;
    NNUE::Networks networks;
    TranspositionTable tt;
};

} // namespace Atom
