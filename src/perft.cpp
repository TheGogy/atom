
#include "movegen.h"
#include "position.h"
#include "types.h"
#include "uci.h"

#include <chrono>
#include <cstddef>
#include <iostream>
#include <fstream>

namespace Atom {

using std::ifstream;

template <bool Div, Color Me>
size_t perft(Position &pos, int depth) {
    size_t total = 0;
    MoveList moves;

    if (!Div && depth <= 1) {
        enumerateLegalMoves<Me>(pos, [&](Move m) {
            ++total;
            return true;
        });

        return total;
    }

    enumerateLegalMoves<Me>(pos, [&](Move move) {
        size_t n = 0;

        if (Div && depth == 1) {
            n = 1;
        } else {
            pos.doMove<Me>(move);
            n = (depth == 1 ? 1 : perft<false, ~Me>(pos, depth - 1));
            pos.undoMove<Me>(move);
        }

        total += n;

        if (Div && n > 0)
            std::cout << Uci::formatMove(move) << ": " << n << std::endl;

        return true;
    });

    return total;
}

template size_t perft<true, WHITE>(Position &pos, int depth);
template size_t perft<false, WHITE>(Position &pos, int depth);
template size_t perft<true, BLACK>(Position &pos, int depth);
template size_t perft<false, BLACK>(Position &pos, int depth);

template<bool Div>
size_t perft(Position &pos, int depth) {
    return pos.getSideToMove() == WHITE ? perft<Div, WHITE>(pos, depth) : perft<Div, BLACK>(pos, depth);
}

template size_t perft<true>(Position &pos, int depth);
template size_t perft<false>(Position &pos, int depth);

void perft(Position &pos, int depth) {
    auto begin = std::chrono::high_resolution_clock::now();
    size_t n = perft<true>(pos, depth);
    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    std::cout << std::endl;
    std::cout << "Nodes:    " << n << std::endl;
    std::cout << "Time:     " << elapsed << "ms" << std::endl;
    std::cout << "NPS:      " << size_t(n * 1000 / elapsed) << std::endl;
}

bool runTest(const std::string &fen, int depth, Bitboard expected_nodes) {
    Position pos;
    pos.setFromFEN(fen);
    Bitboard nodes = perft<false>(pos, depth);
    if (nodes == expected_nodes) {
        std::cout << "[PASS] " << fen << std::endl;
        return true;
    } else {
        std::cout << "[FAIL] " << fen << " || EXPECTED " << expected_nodes << " RETURNED " << nodes << std::endl;
        return false;
    }
}

void testFromFile(const std::string &filename) {
    ifstream file(filename);
    if (!file) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    int tests_passed = 0;
    int total_tests = 0;
    int lines_tested = 0;
    std::string line;

    while (std::getline(file, line)) {

        // Split the line into FEN and perft values
        std::istringstream line_stream(line);
        std::string fen;
        if (!std::getline(line_stream, fen, ';')) continue;

        // Print divider
        std::cout << "\n################################ " << lines_tested++ << " ################################\n\n";

        std::string token;
        while (std::getline(line_stream, token, ';')) {
            token.erase(0, token.find_first_not_of(' '));
            if (token[0] == 'D' && std::isdigit(token[1])) {
                int depth;
                uint64_t expected_nodes;
                if (sscanf(token.c_str(), "D%d %lu", &depth, &expected_nodes) == 2) {
                    // Test the given perft and update counters
                    if (runTest(fen, depth, expected_nodes)) {
                        tests_passed++;
                    }
                    total_tests++;
                }
            }
        }
    }
    file.close();

    // Show results
    std::cout << "\n\n";
    std::cout << "Perft results for " << filename << std::endl;
    std::cout << "Total tests:      " << total_tests << std::endl;
    std::cout << "Tests passed:     " << tests_passed << std::endl;
}

} // namespace Atom
