
#include "movegen.h"
#include "position.h"
#include "types.h"
#include "uci.h"

#include <cstdint>
#include <iostream>
#include <fstream>

namespace Atom {


template <bool Div, Color Me>
std::uint64_t perft(Position &pos, int depth) {
    std::uint64_t total = 0;

    if (!Div && depth <= 1) {
        Movegen::enumerateLegalMoves<Me>(pos, [&](Move m) {
            ++total;
            return true;
        });

        return total;
    }

    Movegen::enumerateLegalMoves<Me>(pos, [&](Move move) {
        std::uint64_t n = 0;

        if (depth == 1) {
            n = Div ? 1 : 0;
        } else {
            pos.doMove<Me>(move);
            n = perft<false, ~Me>(pos, depth - 1);
            pos.undoMove<Me>(move);
        }

        total += n;

        if (Div && n > 0) {
            std::cout << Uci::formatMove(move) << ": " << n << std::endl;
        }

        return true;
    });

    return total;
}

template std::uint64_t perft<true,  WHITE>(Position &pos, int depth);
template std::uint64_t perft<false, WHITE>(Position &pos, int depth);
template std::uint64_t perft<true,  BLACK>(Position &pos, int depth);
template std::uint64_t perft<false, BLACK>(Position &pos, int depth);


template <bool Div>
std::uint64_t perft(Position &pos, int depth) {
    return pos.getSideToMove() == WHITE ? perft<Div, WHITE>(pos, depth) : perft<Div, BLACK>(pos, depth);
}


template std::uint64_t perft<true>(Position &pos, int depth);
template std::uint64_t perft<false>(Position &pos, int depth);


void perft(Position &pos, int depth) {
    long start = now();
    std::uint64_t n = perft<true>(pos, depth);

    long elapsed = now() - start;
    std::cout << std::endl;
    std::cout << "Nodes:    " << n << std::endl;
    std::cout << "Time:     " << elapsed << "ms" << std::endl;

    // Ensure that we do not divide by zero
    if (elapsed > 0) {
        std::cout << "NPS:      " << std::uint64_t(n * 1000 / elapsed) << std::endl;
    } else {
        std::cout << "NPS:      N/A" << std::endl;
    }
}


bool runTest(const std::string &fen, int depth, Bitboard expected) {
    Position pos;
    pos.setFromFEN(fen);
    Bitboard nodes = perft<false>(pos, depth);
    if (nodes == expected) {
        std::cout << "[PASS] " << fen << std::endl;
        return true;
    } else {
        std::cout << "[FAIL] " << fen << " || EXPECTED " << expected << " RETURNED " << nodes << std::endl;
        return false;
    }
}


void testFromFile(const std::string &filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    int passed = 0;
    int total = 0;
    int testedLines = 0;
    std::string line;

    while (std::getline(file, line)) {

        // Split the line into FEN and perft values
        std::istringstream lStream(line);
        std::string fen;
        if (!std::getline(lStream, fen, ';')) continue;

        // Print divider
        std::cout << "\n################################ " << testedLines++ << " ################################\n\n";

        std::string token;
        while (std::getline(lStream, token, ';')) {
            token.erase(0, token.find_first_not_of(' '));
            if (token[0] == 'D' && std::isdigit(token[1])) {
                int depth;
                std::uint64_t expected;
                if (sscanf(token.c_str(), "D%d %lu", &depth, &expected) == 2) {
                    // Test the given perft and update counters
                    if (runTest(fen, depth, expected)) {
                        ++passed;
                    }
                    ++total;
                }
            }
        }
    }
    file.close();

    // Show results
    std::cout << "\n\n";
    std::cout << "Perft results for " << filename << std::endl;
    std::cout << "Total tests:      " << total << std::endl;
    std::cout << "Tests passed:     " << passed << std::endl;
    std::cout << std::endl << std::endl;
}


} // namespace Atom
