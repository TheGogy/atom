
#include "movegen.h"
#include "position.h"
#include "types.h"
#include "uci.h"

#include <chrono>
#include <cstddef>
#include <iostream>
#include <fstream>

using std::ifstream;

/*
 * Runs a perft test on the given position.
 *
 * Returns: 
 *  - total_leaf: The number of leaf nodes (moves generated)
 *  - total_node: The number of nodes that have been expanded (number of times enumerateLegalMoves has been run)
 */
template <bool Div, Side Me>
std::tuple<size_t, size_t> perft(Position &pos, int depth) {
  size_t total_leaf = 0;
  size_t total_node = 0;
  MoveList moves;

  if (!Div && depth <= 1) {
    enumerateLegalMoves<Me>(pos, [&](Move m) {
      ++total_leaf;
      return true;
    });

    ++total_node;

    return std::tuple<size_t, size_t>(total_leaf, total_node);
  }

  enumerateLegalMoves<Me>(pos, [&](Move move) {
      std::tuple<size_t, size_t> n {0, 0};

    if (Div && depth == 1) {
     ++std::get<0>(n);
    } else {
      pos.doMove<Me>(move);
      if (depth == 1) {
      std::get<0>(n) = 1;
      } else {
        n = perft<false, ~Me>(pos, depth - 1);
      }

      pos.undoMove<Me>(move);
    }

    total_leaf += std::get<0>(n);
    total_node += std::get<1>(n);

    if (Div && std::get<0>(n) > 0)
      std::cout << Uci::formatMove(move) << ": " << std::get<0>(n) << std::endl;

    return true;
  });

  ++total_node;

  return std::tuple<size_t, size_t>(total_leaf, total_node);
}

template std::tuple<size_t, size_t> perft<true, WHITE>(Position &pos, int depth);
template std::tuple<size_t, size_t> perft<false, WHITE>(Position &pos, int depth);
template std::tuple<size_t, size_t> perft<true, BLACK>(Position &pos, int depth);
template std::tuple<size_t, size_t> perft<false, BLACK>(Position &pos, int depth);

template<bool Div>
std::tuple<size_t, size_t> perft(Position &pos, int depth) {
    return pos.getSideToMove() == WHITE ? perft<Div, WHITE>(pos, depth) : perft<Div, BLACK>(pos, depth);
}

template std::tuple<size_t, size_t> perft<true>(Position &pos, int depth);
template std::tuple<size_t, size_t> perft<false>(Position &pos, int depth);

void perft(Position &pos, int depth) {
    size_t total_leaf, total_node;
    auto begin = std::chrono::high_resolution_clock::now();
    std::tie(total_leaf, total_node) = perft<true>(pos, depth);
    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    std::cout << std::endl;
    std::cout << "Inner Nodes:      " << total_node << std::endl;
    std::cout << "Leaf nodes:       " << total_leaf << std::endl;
	  std::cout << "Time:             " << elapsed << "ms" << std::endl;
	  std::cout << "Positions / sec:  " << size_t(total_node * 1000 / elapsed) << std::endl;
	  std::cout << "Moves / sec:      " << size_t(total_leaf * 1000 / elapsed) << std::endl;
}

bool runTest(const std::string &fen, int depth, U64 expected_nodes) {
    Position pos;
    pos.setFromFEN(fen);
    size_t nodes = std::get<0>(perft<false>(pos, depth));
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

