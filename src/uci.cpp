
#include "uci.h"
#include "movegen.h"
#include "position.h"
#include "types.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace Atom {

// Formats the given square according to the UCI standard (a1 --> h8)
std::string Uci::formatSquare(Square sq) {
    std::string str;
    str += 'a' + fileOf(sq);
    str += '1' + rankOf(sq);
    return str;
}


// Parses the given square according to UCI standard (a1 --> h8)
Square Uci::parseSquare(std::string str) {
    if (str.length() < 2) return SQ_NONE;

    char col = str[0], row = str[1];
    if ( col < 'a' || col > 'h') return SQ_NONE;
    if ( row < '1' || row > '8') return SQ_NONE;

    return createSquare(File(col - 'a'), Rank(row - '1'));
}


// Formats the given move according to UCI standard.
// This is: <square from><square to><promotion piece>
// e.g "e2e4"
// or  "e7e8q"
//
// Null moves are returned as "0000", although this should
// not happen in normal play.
std::string Uci::formatMove(Move m) {
    if (m == MOVE_NONE) {
        return "(none)";
    } else if (m == MOVE_NULL) {
        return "0000";
    }

    std::string str = formatSquare(moveFrom(m)) + formatSquare(moveTo(m));

    if (moveType(m) == PROMOTION) {
        str += "?pnbrq?"[movePromotionType(m)];
    }

    return str;
}


// Converts a UCI move string into a Move.
// If the move is not in the legal moves, or not a valid move string, returns MOVE_NULL.
Move Uci::toMove(const Position &pos, std::string moveStr) {
    Move m = MOVE_NULL;

    enumerateLegalMoves(pos, [&](Move move) {
        if (moveStr == formatMove(move)) {
            m = move;
        }
        return true;
    });

    return m;
}


// Gets the win rate parameters, taken from stockfish to ensure
// compatibility with NNUE output.
struct WinRateParams {
    double a;
    double b;
};

WinRateParams getWinRateParams(const Position &pos) {
    int material = pos.nPieces(PAWN) +
                   pos.nPieces(KNIGHT) * 3 +
                   pos.nPieces(BISHOP) * 3 +
                   pos.nPieces(ROOK)   * 5 +
                   pos.nPieces(QUEEN)  * 9;

    double m = std::clamp(material, 17, 78) / 58.0;
    constexpr double as[] = {-41.25712052, 121.47473115, -124.46958843, 411.84490997};
    constexpr double bs[] = {84.92998051, -143.66658718, 80.09988253, 49.80869370};

    double a = (((as[0] * m + as[1]) * m + as[2]) * m) + as[3];
    double b = (((bs[0] * m + bs[1]) * m + bs[2]) * m) + bs[3];

    return {a, b};
}

// Gets the current win rate parameters in centipawns. From stockfish.
int Uci::toCentipawns(Value v, const Position &pos) {
    auto [a, b] = getWinRateParams(pos);
    return std::round(100 * int(v) / a);
}


void Uci::loop() {
    std::string token, input;

    while (true) {
        std::getline(std::cin, input);
        std::istringstream is(input);

        token.clear();
        is >> std::skipws >> token;

        if (token == "uci") {
            cmdUci();
        } else if (token == "isready") {
            cmdIsReady();
        } else if (token == "ucinewgame") {
            cmdUciNewGame();
        } else if (token == "setoption") {
            cmdSetOption(is);
        } else if (token == "go") {
            cmdGo(is);
        } else if (token == "stop") {
            cmdStop();
        } else if (token == "perft") {
            cmdPerft(is);
        } else if (token == "debug" || token == "d") {
            cmdDebug();
        } else if (token == "quit") {
            break;
        } else {
            std::cout << "Error: unknown command '" << token << "'" << std::endl;
        }

    }
}


//
// ------------------------------ < UCI main commands > ------------------------------
//
// ╔══════════════════════════════════╦══════════════════════════════════════════════╗
// ║             Command              ║         Response (* means blocking)          ║
// ╠══════════════════════════════════╬══════════════════════════════════════════════╣
// ║ uci                              ║   uciok <engine name, authors, options>      ║
// ║ isready                          ║ * responds with "readyok"                    ║
// ║ ucinewgame                       ║ * resets the TT and all position variables   ║
// ║ setoption name <opt> value <val> ║ * sets the option <opt> to the value <val>   ║
// ║ go (wtime, btime etc)            ║ * Searches current position                  ║
// ║ stop                             ║   Finish search threads and report bestmove  ║
// ║ perft <d>                        ║   Runs perft on current pos up to depth <d>  ║
// ║ debug (or just "d")              ║   Prints the current position + debug info   ║
// ║ quit                             ║   Ends the process                           ║
// ╚══════════════════════════════════╩══════════════════════════════════════════════╝


void Uci::cmdUci() {
    std::cout << "id name Atom " << ENGINE_VERSION << std::endl;
    std::cout << "id author George Rawlinson and Tomáš Pecher" << std::endl;
    std::cout << "uciok" << std::endl;
}


void Uci::cmdIsReady() {
    std::cout << "readyok" << std::endl;
}


void Uci::cmdUciNewGame() {
    engine.newGame();
}


void Uci::cmdSetOption(std::istringstream& is) {
    // TODO: Set option
}


void Uci::cmdGo(std::istringstream& is) {
    // TODO: Choose a random move for debugging
}


void Uci::cmdStop() {
    // TODO: Stop the engine
}


void Uci::cmdPerft(std::istringstream& is) {
    int depth;
    is >> depth;

    if (depth == 0) {
        std::cout << "Please specify a depth > 0." << std::endl;
        return;
    }

    std::cout << "Running perft at depth: " << depth << std::endl;
    engine.runPerft(depth);
}

void Uci::cmdDebug() {
    std::cout << engine.getDebugInfo() << std::endl;
}


void Uci::cmdQuit() {
    engine.stop();
}

} // namespace Atom

