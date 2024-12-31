#pragma once

#include <cstddef>
#include <cstdint>
#include <tuple>

#include "memory.h"
#include "types.h"

namespace Atom {

using Key = uint64_t;

constexpr size_t TT_DEFAULT_SIZE = 16;

constexpr uint8_t ENTRIES_PER_CLUSTER = 3;
constexpr uint8_t DEPTH_DELTA = -3;
constexpr uint8_t BOUND_MASK  = 0b00000011;
constexpr uint8_t PV_MASK     = 0b00000100;
constexpr uint8_t AGE_MASK    = 0b11111000;
constexpr int     AGE_DELTA   = 0x8;
constexpr int     AGE_CYCLE   = 0xFF + AGE_DELTA;


enum Bound {
    BOUND_NONE,
    BOUND_UPPER,
    BOUND_LOWER,
    BOUND_EXACT = BOUND_UPPER | BOUND_LOWER
};


inline Value valueToTT(Value v, int ply) {
    return v >= VALUE_TB_WIN_IN_MAX_PLY ? v + ply : v <= VALUE_TB_LOSS_IN_MAX_PLY ? v - ply : v;
}


struct TTData {
    Move  move;
    Value score, eval;
    Depth depth;
    Bound bound;
    bool  isPv;

    inline Value getAdjustedScore(int ply) {
        return score ==  VALUE_NONE ? VALUE_NONE
             : score >=  VALUE_MATE_IN_MAX_PLY ? score - ply
             : score <= -VALUE_MATE_IN_MAX_PLY ? score + ply
             : score;
    }
};


struct TTEntry {

    inline TTData read() const {
        return TTData{
            .move  = Move (move16),
            .score = Value(score16),
            .eval  = Value(eval16),
            .depth = Depth(depth8 + DEPTH_DELTA),
            .bound = Bound(age8 & BOUND_MASK),
            .isPv  = bool (age8 & PV_MASK),
        };
    }

    inline bool isOccupied() const { return bool(depth8); }

    inline uint8_t relativeAge(const uint8_t age) const {
        return (AGE_CYCLE + age - age8) & AGE_MASK;
    }

    void save(
        Key key, Value score, Value eval,
        Depth depth, bool isPv, Move move,
        uint8_t age, Bound bound
    );

    inline bool hashEquals(Key key) const { return uint16_t(key) == key16; }

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
        Key key, Value score, Value eval,
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
    TranspositionTable(size_t sizeInMb = TT_DEFAULT_SIZE) : table(nullptr), nbClusters(0), age(0) {
        resize(sizeInMb);
    };

    ~TranspositionTable() { aligned_large_pages_free(table); }

    inline TTEntry* lookup(const Key key) const {
        return &table[((unsigned __int128)key * (unsigned __int128)nbClusters) >> 64].entries[0];
    }

    inline void prefetch(Key key) const { __builtin_prefetch(lookup(key)); }

    std::tuple<bool, TTData, TTWriter> probe(const Key key) const;

    // UCI commands
    int    hashfull() const;
    void   clear();
    void   resize(size_t newSize);

    inline void onNewSearch() { age += AGE_DELTA; }

    inline size_t  size()   const { return nbClusters; }
    inline uint8_t getAge() const { return age; }

private:
    TTCluster* table;
    size_t     nbClusters;
    uint8_t    age;
};


} //namespace Atom
