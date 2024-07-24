#ifndef TT_H
#define TT_H

#include <cstddef>
#include <cstdint>
#include <tuple>

#include "memory.h"
#include "types.h"

namespace Atom {

using TTKey = uint64_t;

static constexpr uint8_t ENTRIES_PER_CLUSTER = 3;
static constexpr uint8_t DEPTH_DELTA = -3;
static constexpr uint8_t BOUND_MASK  = 0b00000011;
static constexpr uint8_t PV_MASK     = 0b00000100;
static constexpr uint8_t AGE_MASK    = 0b11111000;
static constexpr int     AGE_DELTA   = 0x8;
static constexpr int     AGE_CYCLE   = 0xFF + AGE_DELTA;

struct TTData {
    Move  move;
    Value score, eval;
    Depth depth;
    Bound bound;
    bool  isPv;
};


struct TTEntry {

    inline TTData read() const {
        return TTData{
            .move  = Move (move16),
            .score = Value(score16),
            .eval  = Value(eval16),
            .depth = Depth(depth8 + DEPTH_DELTA),
            .bound = Bound(depth8 & 0x3),
            .isPv  = bool (depth8 & BOUND_MASK),
        };
    }

    inline bool isOccupied() const { return bool(depth8); }

    inline uint8_t relativeAge(const uint8_t age) const {
        return (AGE_CYCLE + age - age8) & AGE_MASK;
    }

    void save(
        TTKey key, Value score, Value eval,
        Depth depth, bool isPv, Move move,
        uint8_t age, Bound bound
    );

    inline bool hashEquals(TTKey key) const { return uint16_t(key) == key16; }

    inline uint8_t age() const { return age8 & AGE_MASK; }

    inline bool isBetterThan(const TTEntry &other, uint8_t age) const {
        return (
            this->depth8 - this->relativeAge(age) * 2
          > other.depth8 - other.relativeAge(age) * 2
        );
    }

private:
    friend class TranspositionTable;

    // Keep these in this order
    uint16_t key16;
    uint8_t  depth8;
    uint8_t  age8;
    Move     move16;
    int16_t  score16;
    int16_t  eval16;
};


struct TTWriter {
public:
    void write(
        TTKey key, Value score, Value eval,
        Depth depth, bool isPv, Move move,
        uint8_t age, Bound bound
    );
 
private:
    friend class TranspositionTable;
    TTWriter(TTEntry *entry);
    TTEntry *entry;
};


class TTCluster {
    inline TTEntry *begin() { return &entries[0]; }
    inline TTEntry *end()   { return &entries[ENTRIES_PER_CLUSTER]; }

    friend class TranspositionTable;
    TTEntry  entries[ENTRIES_PER_CLUSTER]; // (10 * 3) bytes
    uint16_t padding;               // 2 bytes
};  // Must be exactly 32 bytes


class TranspositionTable {
public:
    TranspositionTable(size_t sizeInMb);
    ~TranspositionTable() { aligned_large_pages_free(table); }

    inline TTEntry* lookup(const TTKey key) const {
        return &table[((unsigned __int128)key * (unsigned __int128)nClusters) >> 64].entries[0];
    }

    inline void prefetch(TTKey key) const { __builtin_prefetch(lookup(key)); }

    std::tuple<bool, TTData, TTWriter> probe(const TTKey key) const;

    // UCI commands
    int    hashfull() const;
    void   clear();
    void   resize(size_t newSize);

    inline size_t size() const { return nClusters; }
    inline void onNewSearch() { age += AGE_DELTA; }

private:
    TTCluster* table;
    size_t     nClusters;
    uint8_t    age;
};


} //namespace Atom

#endif // TT_H
