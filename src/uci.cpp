
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <string>
#include <string_view>

#include "uci.h"
#include "movegen.h"
#include "nnue.h"
#include "position.h"
#include "search.h"
#include "types.h"

namespace Atom {


std::string Uci::toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](auto c) { 
        return std::tolower(c); 
    });

    return s;
}


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

    Movegen::enumerateLegalMoves(pos, [&](Move move) {
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


std::string Uci::formatScore(const Value& score, const Position& pos) {
    constexpr int TB_TO_CP = 20000;

    assert(-VALUE_INFINITE < score && VALUE_INFINITE > score);

    const Value absScore = abs(score);

    // Regular score
    if (absScore < VALUE_TB_WIN_IN_MAX_PLY) {
        Value val = Uci::toCentipawns(score, pos);
        return std::string("cp ") + std::to_string(val);
    }

    // Tablebase score
    else if (absScore <= VALUE_TB) {
        int ply   = VALUE_TB - absScore;
        Value val = score > 0 ? TB_TO_CP - ply : -TB_TO_CP + ply;
        return std::string("cp ") + std::to_string(val);
    }

    // Forced checkmate
    else {
        int ply = VALUE_MATE - absScore;
        int val = score > 0 ? ((ply + 1) / 2) : (ply / 2);
        return std::string("mate ") + std::to_string(val);
    }
}


//
// UCI callbacks
//

void Uci::callbackBestMove(const std::string_view bestmove, const std::string_view ponder) {
    std::cout << "bestmove " << bestmove;
    if (!ponder.empty()) std::cout << " ponder " << ponder;
    std::cout << std::endl;
}


void Uci::callbackInfo(const Search::SearchInfo info) {
    std::stringstream ss;

    ss << "info";
    ss << " depth "    << info.depth
       << " seldepth " << info.selDepth
       << " score "    << info.score
       << " nodes "    << info.nodesSearched
       << " nps "      << (info.nodesSearched * 1000) / info.timeSearched
       << " hashfull " << info.hashFull
       << " tbhits "   << info.tbHits
       << " time "     << info.timeSearched
       << " pv "       << info.pv;

    std::cout << ss.str() << std::endl;
}

//
//  Main UCI loop.
//

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
        } else if (token == "position") {
            cmdPosition(is);
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
        } else if (token == "clear") {
            std::cout << "\033[2J\033[1;1H";
        } else if (token == "visualize" || token == "v") {
            cmdVisualize(is);
        } else {
            std::cout << "Error: unknown command '" << token << "'" << std::endl;
        }
    }
}


// ------------------------------ < UCI main commands > ------------------------------
//
// +-----------------------------------+----------------------------------------------+
// |             Command               |         Response (* means blocking)          |
// +-----------------------------------+----------------------------------------------+
// | uci                               |   uciok <engine name, authors, options>      |
// | isready                           | * Responds with "readyok"                    |
// | ucinewgame                        | * Resets the TT and all position variables   |
// | position <fen / startpos> <moves> | * Sets the position according to FEN / moves |
// | setoption name <opt> value <val>  | * Sets the option <opt> to the value <val>   |
// | go (wtime, btime etc)             | * Searches current position                  |
// | stop                              |   Finish search threads and report bestmove  |
// | perft <depth>                     |   Runs perft on current pos to given depth   |
// | debug (or just "d")               |   Prints the current position + debug info   |
// | quit                              |   Ends the process                           |
// | clear                             |   Clears the terminal                        |
// | visualize (or just "v") <bb>      |   Prints a visualization of bitboard <bb>    |
// | eval                              |   Traces the evaluation of the current pos   |
// +-----------------------------------+----------------------------------------------+


void Uci::cmdUci() {
    std::cout << "id name Atom " << ENGINE_VERSION << std::endl;
    std::cout << "id author George Rawlinson and Tomáš Pecher" << std::endl;
    std::cout << std::endl;
    std::cout << "option name EvalFile type string default <inbuilt> " << EvalFileDefaultNameBig << std::endl;
    std::cout << "option name EvalFileSmall type string default <inbuilt> " << EvalFileDefaultNameSmall << std::endl;
    std::cout << "option name Hash type spin default 16 min 1 max 4096" << std::endl;
    std::cout << "option name Clear Hash type button" << std::endl;
    std::cout << "uciok" << std::endl;
}


void Uci::cmdIsReady() {
    std::cout << "readyok" << std::endl;
}


void Uci::cmdUciNewGame() {
    engine.newGame();
}


void Uci::cmdPosition(std::istringstream& is) {
    std::string token, fen;

    is >> token;

    // Set position according to position token
    // This bit consumes up to the moves token
    if (token == "startpos") {
        fen = STARTPOS_FEN;
        is >> token;
    } else if (token == "kiwipete") {
        fen = KIWIPETE_FEN;
        is >> token;
    } else if (token == "fen") {
        while (is >> token && token != "moves") {
            fen += token + ' ';
        }
    } else {
        std::cout << "Error: unknown position" << std::endl;
        return;
    }

    // This bit consumes all the moves
    std::vector<std::string> moves;
    while (is >> token) {
        moves.push_back(toLower(token));
    }

    engine.setPosition(fen, moves);
}


void Uci::cmdSetOption(std::istringstream& is) {
    // +---------------+---------------------------------------+---------------+
    // |  Option       |                Value                  |    Default    |
    // +---------------+---------------------------------------+---------------+
    // | EvalFile      | (string)  Path to the big NNUE file   | <inbuilt>     |
    // | EvalFileSmall | (string)  Path to the small NNUE file | <inbuilt>     |
    // | Hash          | (spin)    Hash size, in MB            | 16            |
    // | ClearHash     | (button)  Clears the hash             |               |
    // | Threads       | (spin)    Number of threads to use    | 1             |
    // +---------------+---------------------------------------+---------------+

    // INFO: The logic here may need to be reworked should we decide to add any options
    // that contain spaces in the name.

    // Do not change the options mid search
    engine.waitForSearchFinish();

    std::string token, optName, value;
    is >> token;
    if (token != "name") {
        std::cout << "Error: unknown format. Should use 'setoption name <option> ...'" << std::endl;
        return;
    }

    is >> optName;
    is >> value;

    if (value == "value") {
        is >> token;
        if (optName == "EvalFile") {
            engine.loadBigNetFromFile(token);
        } else if (optName == "EvalFileSmall") {
            engine.loadSmallNetFromFile(token);
        } else if (optName == "Hash") {
            engine.setHashSize(std::stoi(token));
        } else if (optName == "Threads") {
            engine.setNbThreads(std::stoi(token));
        } else {
            std::cout << "Error: Unknown option name." << std::endl;
        }
        return;
    } else if (value == "ClearHash") {
        engine.clear();
    }

}

Search::SearchLimits Uci::parseGoLimits(std::istringstream& is) {
    Search::SearchLimits limits;
    std::string token;

    limits.startTimePoint = now();

    while (is >> token) {
        if (token == "searchmoves") {           // Restrict search to specific moves.
            while (is >> token) limits.searchMoves.push_back(toLower(token));

        } else if (token == "wtime") {          // White has <x> ms left on clock
            is >> limits.time[WHITE];
        } else if (token == "btime") {          // Black has <x> ms left on clock
            is >> limits.time[BLACK];
        } else if (token == "winc") {           // White has <x> ms increment
            is >> limits.inc[WHITE];
        } else if (token == "binc") {           // Black has <x> ms increment
            is >> limits.inc[BLACK];
        } else if (token == "movestogo") {      // There are <x> ms until next time control
            is >> limits.movesToGo;
        } else if (token == "depth") {          // Search up to depth <x>
            is >> limits.depth;
        } else if (token == "nodes") {          // Search only <x> nodes
            is >> limits.nodes;
        } else if (token == "mate") {           // Search for mate in <x> moves
            is >> limits.mate;
        } else if (token == "movetime") {       // Search for <x> milliseconds
            is >> limits.moveTime;
        } else if (token == "infinite") {       // Search infinitely until stop command called
            limits.isInfinite = true;
        }
    }

    return limits;
}


void Uci::cmdGo(std::istringstream& is) {
    Search::SearchLimits limits = parseGoLimits(is);
    engine.go(limits);
}


void Uci::cmdStop() {
    // TODO: return bestmove
    engine.stop();
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


void Uci::cmdVisualize(std::istringstream& is) {
    std::string token;
    is >> token;
    token = toLower(token);

    if (token == "pinortho") {
        std::cout << engine.visualizePinOrtho() << std::endl;
    } else if (token == "pindiag") {
        std::cout << engine.visualizePinDiag() << std::endl;
    } else if (token == "checkers") {
        std::cout << engine.visualizeCheckers() << std::endl;
    } else if (token == "checkmask") {
        std::cout << engine.visualizeCheckmask() << std::endl;
    } else if (token == "threatened") {
        std::cout << engine.visualizeThreatened() << std::endl;
    } else {
        std::cout << "Error: Unknown bitboard to visualize" << std::endl;
    }
}


void Uci::cmdTraceEval() {
    engine.traceEval();
}



} // namespace Atom

