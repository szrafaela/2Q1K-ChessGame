#include "Board.h"
#include "Piece.h"
#include <memory>

Board::Board() {
    board.resize(8, std::vector<std::shared_ptr<Piece>>(8, nullptr));
}

void Board::initialize() {
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            board[y][x] = nullptr;
        }
    }

    // Minimal starting setup: kings and pawns only.
    board[0][4] = Piece::create(PieceType::King, Color::White, 4, 0);
    board[7][4] = Piece::create(PieceType::King, Color::Black, 4, 7);

    for (int x = 0; x < 8; ++x) {
        board[1][x] = Piece::create(PieceType::Pawn, Color::White, x, 1);
        board[6][x] = Piece::create(PieceType::Pawn, Color::Black, x, 6);
    }
}

std::shared_ptr<Piece> Board::getPieceAt(int x, int y) const {
    if (!isInsideBoard(x, y)) return nullptr;
    return board[y][x];
}

void Board::setPieceAt(int x, int y, std::shared_ptr<Piece> piece) {
    if (!isInsideBoard(x, y)) return;
    board[y][x] = piece;
}

void Board::movePiece(int fromX, int fromY, int toX, int toY) {
    auto piece = getPieceAt(fromX, fromY);
    if (piece && isInsideBoard(toX, toY)) {
        setPieceAt(toX, toY, piece);
        setPieceAt(fromX, fromY, nullptr);
        piece->setPosition(toX, toY);
    }
}

bool Board::isValidMove(std::shared_ptr<Piece> piece, int toX, int toY) const {
    if (!piece) return false;
    if (!isInsideBoard(toX, toY)) return false;
    if (!isInsideBoard(piece->getX(), piece->getY())) return false;
    if (piece->getX() == toX && piece->getY() == toY) return false;
    auto destPiece = getPieceAt(toX, toY);
    if (destPiece && destPiece->getColor() == piece->getColor()) return false;

    return piece->isValidMove(*this, toX, toY);
}

bool Board::isInsideBoard(int x, int y) const {
    return x >= 0 && x < 8 && y >= 0 && y < 8;
}
