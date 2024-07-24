#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <tuple>

#include "tt.h"
#include "memory.h"
#include "types.h"

namespace Atom {

// Updates a TTEntry with new data.
// This can be racy.
void TTEntry::save(
    TTKey key, Value score, Value eval,
    Depth depth, bool isPv, Move move,
    uint8_t age, Bound bound
) {

    if (move != MOVE_NONE || !hashEquals(key)) {
        move16 = move;
    }

    // Check if the new entry is more valuable than the current one
    if (
        bound == BOUND_EXACT ||
        !hashEquals(key)     ||
        (depth - DEPTH_DELTA + 2*isPv > depth8 - 4) ||
        relativeAge(age)
    ) {
        key16   = uint16_t(key);
        depth8  = uint8_t(depth - DEPTH_DELTA);
        age8    = uint8_t(age | (uint8_t(isPv) << 2) | bound);
        score16 = int16_t(score);
        eval16  = int16_t(eval);
    }
}


TTWriter::TTWriter(TTEntry* entry) : entry(entry) {}

void TTWriter::write(
    TTKey key, Value score, Value eval,
    Depth depth, bool isPv, Move move,
    uint8_t age, Bound bound
) {
    entry->save(key, score, eval, depth, isPv, move, age, bound);
}


std::tuple<bool, TTData, TTWriter> TranspositionTable::probe(const TTKey key) const {
    TTEntry* const entry = lookup(key);
    const uint16_t key16 = uint16_t(key);

    for (int i = 0; i < ENTRIES_PER_CLUSTER; ++i) {
        if (entry[i].key16 == key16) {
            return {entry[i].isOccupied(), entry[i].read(), TTWriter(&entry[i])};
        }
    }

    TTEntry* replace = entry;
    for (int i = 1; i < ENTRIES_PER_CLUSTER; ++i) {
        if (replace->isBetterThan(entry[i], age)) {
            replace = &entry[i];
        }
    }

    return {false, TTData(), TTWriter(replace)};
}


// Only reads the first 1000 samples.
int TranspositionTable::hashfull() const {
    int count = 0;
    for (int i = 0; i < 1000; ++i) {
        for (int j = 0 ; j < ENTRIES_PER_CLUSTER; ++j) {
            const TTEntry entry = table[i].entries[j];
            count += entry.isOccupied() && (entry.age() == age);
        }
    }
    return count / ENTRIES_PER_CLUSTER;
}


void TranspositionTable::clear() {
    age = 0;
    std::memset(table, 0, nClusters * sizeof(TTCluster));
}


void TranspositionTable::resize(size_t newSize) {
    aligned_large_pages_free(table);

    nClusters = (newSize * 1024 * 1024) / sizeof(TTCluster);
    table = static_cast<TTCluster*>(aligned_large_pages_alloc(nClusters * sizeof(TTCluster)));

    if (!table) {
        std::cerr << "Failed to allocate transposition table with " << newSize << "MB." << std::endl;
        exit(EXIT_FAILURE);
    }

    clear();
}

} // namespace Atom
