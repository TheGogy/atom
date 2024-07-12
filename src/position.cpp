
#include "position.h"
#include "bitboards.h"
#include "types.h"
#include "zobrist.h"
#include "uci.h"

#include <cstring>
#include <iostream>
#include <sstream>

const std::string PIECE_TO_CHAR(" PNBRQK  pnbrqk");

inline void Position::updateBitboards() { 
    sideToMove == WHITE ? updateBitboards<WHITE>() : updateBitboards<BLACK>(); 
}

template<Side Me>
inline void Position::updateBitboards() {
    updateThreatened<Me>();
    updateCheckers<Me>();
    checkers() ? updatePinsAndCheckMask<Me, true>() : updatePinsAndCheckMask<Me, false>();
}

template<Side Me>
inline void Position::updateThreatened() {
    constexpr Side Opp = ~Me;
    const U64 occ = getPiecesBB() ^ getPiecesBB(Me, KING);
    U64 threatened, enemies;

    // Pawns
    threatened = allPawnAttacks<Opp>(getPiecesBB(Opp, PAWN));

    // Knights
    enemies = getPiecesBB(Opp, KNIGHT);
    bitloop(enemies) threatened |= KNIGHT_MOVE[bitscan(enemies)];

    // Bishops + queens
    enemies = getPiecesBB(Opp, BISHOP, QUEEN);
    bitloop(enemies) threatened |= attacks<BISHOP>(bitscan(enemies), occ);

    // Rooks + queens
    enemies = getPiecesBB(Opp, ROOK, QUEEN);
    bitloop(enemies) threatened |= attacks<ROOK>(bitscan(enemies), occ);

    // King
    threatened |= attacks<KING>(getKingSquare(Opp));

    state->attacked = threatened;
}

template<Side Me, bool InCheck>
inline void Position::updatePinsAndCheckMask() {
    constexpr Side Opp = ~Me;
    const Square ksq = getKingSquare(Me);
    const U64 opp_occ = getPiecesBB(Opp);
    const U64 my_occ = getPiecesBB(Me);
    Square s;
    U64 pinDiag = EMPTY, pinOrtho = EMPTY;
    U64 checkmask, between;

    // Pawns and knights
    if constexpr (InCheck) {
        checkmask = (pawnAttacks(Me, ksq) & getPiecesBB(Opp, PAWN))
                  | (attacks<KNIGHT>(ksq) & getPiecesBB(Opp, KNIGHT));
    }

    // Bishops and queens
    U64 pinners = (attacks<BISHOP>(ksq, opp_occ) & getPiecesBB(Opp, BISHOP, QUEEN));
    bitloop(pinners) {
        s = bitscan(pinners);
        between = BETWEEN_BB[ksq][s];

        switch(popcount(between & my_occ)) {
            case 0: if constexpr (InCheck) checkmask |= between | sq_to_bb(s); break;
            case 1: pinDiag |= between | sq_to_bb(s);
        }
    }

    // Rooks and queens
    pinners = (attacks<ROOK>(ksq, opp_occ) & getPiecesBB(Opp, ROOK, QUEEN));
    bitloop(pinners) {
        s = bitscan(pinners);
        between = BETWEEN_BB[ksq][s];

        switch(popcount(between & my_occ)) {
            case 0: if constexpr (InCheck) checkmask |= between | sq_to_bb(s); break;
            case 1: pinOrtho |= between | sq_to_bb(s);
        }
    }

    state->pinDiag = pinDiag;
    state->pinOrtho = pinOrtho;
    if constexpr (InCheck) state->checkMask = checkmask;
}

template<Side Me>
inline void Position::updateCheckers() {
    const Square ksq = getKingSquare(Me);
    constexpr Side Opp = ~Me;
    const U64 occ = getPiecesBB();

    state->checkers = ((pawnAttacks(Me, ksq) & getPiecesBB(Opp, PAWN))
        | (attacks<KNIGHT>(ksq) & getPiecesBB(Opp, KNIGHT))
        | (attacks<BISHOP>(ksq, occ) & getPiecesBB(Opp, BISHOP, QUEEN))
        | (attacks<ROOK>(ksq, occ) & getPiecesBB(Opp, ROOK, QUEEN))
    );
}

U64 Position::computeHash() const {
    U64 hash = 0;

    for (Square sq = FIRST_SQUARE; sq < N_SQUARES; ++sq) {
        Piece p = getPieceAt(sq);
        if (p == NO_PIECE) continue;

        hash ^= zobrist_keys[p][sq];
    }

    hash ^= zobrist_castlingKeys[getCastlingRights()];
    if (getEpSquare() != NO_SQUARE) hash ^= zobrist_enpassantKeys[fileOf(getEpSquare())];
    if (getSideToMove() == BLACK) hash ^= zobrist_sideToMoveKey;

    return hash;
}

char pieceToChar(Piece p) {
    return PIECE_TO_CHAR[p];
}

Piece charToPiece(char c) {
    return Piece(PIECE_TO_CHAR.find(c));
}

Position::Position() {
    setFromFEN(STARTPOS_FEN);
}

Position::Position(const Position &other) {
    std::memcpy(this, &other, sizeof(Position));
    this->state = this->history + (other.state - other.history);
}

Position& Position::operator=(const Position &other) {
    std::memcpy(this, &other, sizeof(Position));
    this->state = this->history + (other.state - other.history);

    return *this;
}

void Position::reset() {
    state = &history[0];

    state->fiftyMoveRule = 0;
    state->halfMoves = 0;
    state->epSquare = NO_SQUARE;
    state->castlingRights = NO_CASTLING;
    state->move = MOVE_NONE;

    for(int i = 0; i < N_SQUARES; i++) pieces[i]   = NO_PIECE;
    for(int i = 0; i < N_PIECES;  i++) piecesBB[i] = EMPTY;

    sideBB[WHITE] = sideBB[BLACK] = EMPTY;
    sideToMove = WHITE;
}

std::string Position::fen() const {
    std::ostringstream ss;
    int empty;

    // Position part of fen
    for (int r = RANK_8; r >= RANK_1; --r) {
        for (int f = FILE_A; f <= FILE_H; ++f) {
            empty = 0;

            while (f <= FILE_H && isEmpty(makeSquare(File(f), Rank(r)))) {
                ++empty;
                ++f;
            }

            if (empty) ss << empty;
            if (f <= FILE_H) ss << PIECE_TO_CHAR[getPieceAt(makeSquare(File(f), Rank(r)))];
        }

        if (r > RANK_1) ss << '/';
    }

    // Side to move
    ss << (sideToMove == WHITE ? " w " : " b ");

    // Castling rights
    if (canCastle(WHITE_KING_SIDE))  ss << 'K';
    if (canCastle(WHITE_QUEEN_SIDE)) ss << 'Q';
    if (canCastle(BLACK_KING_SIDE))  ss << 'k';
    if (canCastle(BLACK_KING_SIDE))  ss << 'q';
    if (!canCastle(ALL_CASTLING))    ss << '-';

    // En passant square
    ss << (getEpSquare() == NO_SQUARE ? " - " : " " + Uci::formatSquare(getEpSquare()) + " ");

    // Halfmove + fullmove clock
    ss << getHalfMoveClock() << " " << getFullMoves();

    return ss.str();
}

std::string Position::printable() const {
    std::ostringstream ss;
    ss << "   ╔═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╗" << std::endl;
    for (int r = RANK_8; r >= RANK_1; --r) {
        ss << " " << r + 1 << " ║ ";
        for (int f = FILE_A; f <= FILE_H; ++f) {
            ss << PIECE_TO_CHAR[getPieceAt(makeSquare(File(f), Rank(r)))];
            ss << (f < FILE_H ? " │ " : " ║ ");
        }
        if (r > RANK_1)
            ss << std::endl << "   ╟───┼───┼───┼───┼───┼───┼───┼───╢" << std::endl;
    }
    ss << std::endl << "   ╚═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╝" << std::endl;
    ss << std::endl << "     a   b   c   d   e   f   g   h" << std::endl;

    return ss.str();
}

bool Position::setFromFEN(const std::string &fen) {
    reset();

    std::istringstream parser(fen);
    std::string token;

    // Process piece placement from FEN
    parser >> std::skipws >> token;
    int file = 0, rank = 7;
    for (char c : token) {
        if (c == '/') {
            // Move to the next rank
            rank--;
            file = 0;
        } else if (std::isdigit(c)) {
            // Skip the specified number of files
            file += c - '0';
        } else {
            // Place the piece on the board
            Piece piece = charToPiece(c);
            Square s = makeSquare(File(file), Rank(rank));

            if (!isValidPiece(piece) || !isValidSq(s)) {
                std::cout << "Invalid FEN (invalid piece / square)" << std::endl;
                reset();
                return false;
            }

            sideOf(piece) == WHITE ? setPiece<WHITE>(s, piece) : setPiece<BLACK>(s, piece);
            file++;
        }
    }
    if (file != 8 || rank != 0) {
        std::cout << "Invalid FEN (incorrect piece placement)" << std::endl;
    }

    // Process side to move
    parser >> std::skipws >> token;
    switch (token[0]) {
        case 'w': sideToMove = WHITE; break;
        case 'b': sideToMove = BLACK; break;
        default:
            reset();
            return false;
    }

    // Process castling rights
    parser >> std::skipws >> token;
    for (char c : token) {
        switch (c) {
            case 'K': state->castlingRights |= WHITE_KING_SIDE;  break;
            case 'Q': state->castlingRights |= WHITE_QUEEN_SIDE; break;
            case 'k': state->castlingRights |= BLACK_KING_SIDE;  break;
            case 'q': state->castlingRights |= BLACK_QUEEN_SIDE; break;
            case '-': break;
            default:
                reset();
                return false;
        }
    }

    // Process en passant square
    parser >> std::skipws >> token;
    state->epSquare = Uci::parseSquare(token);
    if (state->epSquare != NO_SQUARE && !isValidSq(state->epSquare)) {
        reset();
        return false;
    }

    // Process fifty-move rule counter
    parser >> std::skipws >> state->fiftyMoveRule;
    if (state->fiftyMoveRule < 0) {
        reset();
        return false;
    }

    // Process full move counter
    parser >> std::skipws >> state->halfMoves;
    state->halfMoves = std::max(2 * (state->halfMoves - 1), 0) + (sideToMove == BLACK);
    if (state->halfMoves < 0) {
        reset();
        return false;
    }

    // Update bitboards and hash
    updateBitboards();
    this->state->hash = computeHash();

    return true;
}

template<Side Me>
inline void Position::setPiece(Square sq, Piece p) {
    U64 b = sq_to_bb(sq);
    pieces[sq] = p;
    sideBB[Me]  |= b;
    piecesBB[p] |= b;
}

template<Side Me>
inline void Position::unsetPiece(Square sq) {
    U64 b = sq_to_bb(sq);
    Piece p = pieces[sq];
    pieces[sq] = NO_PIECE;
    sideBB[Me]  &= ~b;
    piecesBB[p] &= ~b;
}

template<Side Me>
inline void Position::movePiece(Square from, Square to) {
    U64 fromTo = from | to;
    Piece p = pieces[from];
    pieces[to] = p;
    pieces[from] = NO_PIECE;
    sideBB[Me]  ^= fromTo;
    piecesBB[p] ^= fromTo;
}

template<Side Me, MoveType Mt>
void Position::doMove(Move m) {
    const Square from = moveFrom(m);
    const Square to = moveTo(m);
    const Piece p = getPieceAt(from);
    const Piece capture = getPieceAt(to);

    uint64_t hash = state->hash;

    // Update the hash to remove en passant square
    hash ^= zobrist_enpassantKeys[fileOf(state->epSquare) + N_FILES * (state->epSquare == NO_SQUARE)];

    // Save the current board state and advance to the next state
    BoardState *oldState = state++;
    state->epSquare = NO_SQUARE;
    state->castlingRights = oldState->castlingRights;
    state->fiftyMoveRule = oldState->fiftyMoveRule + 1;
    state->halfMoves = oldState->halfMoves + 1;
    state->captured = capture;
    state->move = m;

    if constexpr (Mt == NORMAL) {
        // Handle normal moves
        if (capture != NO_PIECE) {
            // Update hash for the captured piece
            hash ^= zobrist_keys[capture][to];
            // Remove the captured piece
            unsetPiece<~Me>(to);
            // Reset the fifty-move rule counter
            state->fiftyMoveRule = 0;
        }

        // Update hash for the moving piece
        hash ^= zobrist_keys[p][from] ^ zobrist_keys[p][to];
        // Move the piece on the board
        movePiece<Me>(from, to);

        // Update castling rights
        hash ^= zobrist_castlingKeys[state->castlingRights];
        state->castlingRights &= ~(CastlingRightsMask[from] | CastlingRightsMask[to]);
        hash ^= zobrist_castlingKeys[state->castlingRights];

        // Special handling for pawn moves
        if (p == makePiece(Me, PAWN)) {
            state->fiftyMoveRule = 0;

            // Set en passant square on double pawn push
            if ((int(from) ^ int(to)) == int(UP + UP)) {
                const Square epsq = to - pawnDirection(Me);

                // Check if opponent pawn can capture en passant
                if (pawnAttacks(Me, epsq) & getPiecesBB(~Me, PAWN)) {
                    hash ^= zobrist_enpassantKeys[fileOf(epsq)];
                    state->epSquare = epsq;
                }
            }
        }
    } else if constexpr (Mt == CASTLING) {
        // Handle castling moves
        CastlingRight cr = Me & (to > from ? KING_SIDE : QUEEN_SIDE);
        const Square rookFrom = CastlingRookFrom[cr];
        const Square rookTo = CastlingRookTo[cr];

        // Update hash for the king and rook moves
        hash ^= zobrist_keys[makePiece(Me, KING)][from] ^ zobrist_keys[makePiece(Me, KING)][to];
        hash ^= zobrist_keys[makePiece(Me, ROOK)][rookFrom] ^ zobrist_keys[makePiece(Me, ROOK)][rookTo];

        // Move the king and rook on the board
        movePiece<Me>(from, to);
        movePiece<Me>(rookFrom, rookTo);

        // Update castling rights
        hash ^= zobrist_castlingKeys[state->castlingRights];
        state->castlingRights &= ~(CastlingRightsMask[from] | CastlingRightsMask[to]);
        hash ^= zobrist_castlingKeys[state->castlingRights];
    } else if constexpr (Mt == PROMOTION) {
        // Handle pawn promotion moves
        const PieceType promotionType = movePromotionType(m);

        // Update hash for the captured piece
        if (capture != NO_PIECE) {
            hash ^= zobrist_keys[capture][to];
            // Remove the captured piece
            unsetPiece<~Me>(to);
        }

        // Update hash for the pawn and promoted piece
        hash ^= zobrist_keys[makePiece(Me, PAWN)][from] ^ zobrist_keys[makePiece(Me, promotionType)][to];
        // Remove the pawn and set the promoted piece
        unsetPiece<Me>(from);
        setPiece<Me>(to, makePiece(Me, promotionType));
        // Reset the fifty-move rule counter
        state->fiftyMoveRule = 0;

        // Update castling rights
        hash ^= zobrist_castlingKeys[state->castlingRights];
        state->castlingRights &= ~(CastlingRightsMask[from] | CastlingRightsMask[to]);
        hash ^= zobrist_castlingKeys[state->castlingRights];
    } else if constexpr (Mt == EN_PASSANT) {
        // Handle en passant captures
        const Square epsq = to - pawnDirection(Me);

        // Update hash for the captured pawn and the moving pawn
        hash ^= zobrist_keys[makePiece(~Me, PAWN)][epsq];
        hash ^= zobrist_keys[makePiece(Me, PAWN)][from] ^ zobrist_keys[makePiece(Me, PAWN)][to];

        // Remove the captured pawn and move the capturing pawn
        unsetPiece<~Me>(epsq);
        movePiece<Me>(from, to);

        // Reset the fifty-move rule counter
        state->fiftyMoveRule = 0;
    }

    // Update hash for the side to move
    hash ^= zobrist_sideToMoveKey;
    state->hash = hash;
    sideToMove = ~Me;
    updateBitboards<~Me>();
}


template void Position::doMove<WHITE, NORMAL>(Move m);
template void Position::doMove<WHITE, PROMOTION>(Move m);
template void Position::doMove<WHITE, EN_PASSANT>(Move m);
template void Position::doMove<WHITE, CASTLING>(Move m);
template void Position::doMove<BLACK, NORMAL>(Move m);
template void Position::doMove<BLACK, PROMOTION>(Move m);
template void Position::doMove<BLACK, EN_PASSANT>(Move m);
template void Position::doMove<BLACK, CASTLING>(Move m);

template<Side Me, MoveType Mt>
void Position::undoMove(Move m) {
    const Square from = moveFrom(m);
    const Square to = moveTo(m);
    const Piece capture = state->captured;

    state--;
    sideToMove = Me;

    if constexpr (Mt == NORMAL) {
        movePiece<Me>(to, from);

        if (capture != NO_PIECE) {
            setPiece<~Me>(to, capture);
        }
    } else if constexpr (Mt == CASTLING) {
        const CastlingRight cr = Me & (to > from ? KING_SIDE : QUEEN_SIDE);
        const Square rookFrom = CastlingRookFrom[cr];
        const Square rookTo = CastlingRookTo[cr];

        movePiece<Me>(to, from);
        movePiece<Me>(rookTo, rookFrom);
    } else if constexpr (Mt == PROMOTION){
        unsetPiece<Me>(to);
        setPiece<Me>(from, makePiece(Me, PAWN));

        if (capture != NO_PIECE) {
            setPiece<~Me>(to, capture);
        }
    } else if constexpr (Mt == EN_PASSANT) {
        movePiece<Me>(to, from);

        const Square epsq = to - pawnDirection(Me);
        setPiece<~Me>(epsq, makePiece(~Me, PAWN));
    }
}

template void Position::undoMove<WHITE, NORMAL>(Move m);
template void Position::undoMove<WHITE, PROMOTION>(Move m);
template void Position::undoMove<WHITE, EN_PASSANT>(Move m);
template void Position::undoMove<WHITE, CASTLING>(Move m);
template void Position::undoMove<BLACK, NORMAL>(Move m);
template void Position::undoMove<BLACK, PROMOTION>(Move m);
template void Position::undoMove<BLACK, EN_PASSANT>(Move m);
template void Position::undoMove<BLACK, CASTLING>(Move m);

void print_position(const Position &pos) {
    std::cout << pos.printable() << std::endl << std::endl;
    std::cout << "FEN:            " << pos.fen() << std::endl;
    std::cout << "Hash:           " << pos.hash() << std::endl;
    std::cout << "Diagonal pin:   " << pos.pinDiag() << std::endl;
    std::cout << "Orthogonal pin: " << pos.pinOrtho() << std::endl;
    std::cout << "Checkmask:      " << pos.checkMask() << std::endl;
}
