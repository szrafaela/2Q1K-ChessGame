#include "Board.h"
#include "Piece.h"
#include <memory>

Board::Board() {
    board.resize(8, std::vector<std::shared_ptr<Piece>>(8, nullptr));
}

void Board::initialize() {
    // Ürítjük a táblát
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x)
            board[y][x] = nullptr;

    // Fehér király az alsó sorban (4, 0)
    board[0][4] = std::make_shared<Piece>(PieceType::King, Color::White, 4, 0);

    // Fekete király a felső sorban (4, 7)
    board[7][4] = std::make_shared<Piece>(PieceType::King, Color::Black, 4, 7);

    for (int x = 0; x < 8; ++x) {
        board[1][x] = std::make_shared<Piece>(PieceType::Pawn, Color::White, x, 1);
        board[6][x] = std::make_shared<Piece>(PieceType::Pawn, Color::Black, x, 6);
    }
}

std::shared_ptr<Piece> Board::getPieceAt(int x, int y) const {
    return board[y][x];
}

void Board::setPieceAt(int x, int y, std::shared_ptr<Piece> piece) {
    board[y][x] = piece;
}

void Board::movePiece(int fromX, int fromY, int toX, int toY) {
    auto piece = getPieceAt(fromX, fromY);
    if (piece) {
        setPieceAt(toX, toY, piece);
        setPieceAt(fromX, fromY, nullptr);
        piece->setPosition(toX, toY);
    }
}

bool Board::isValidMove(std::shared_ptr<Piece> piece, int toX, int toY) const {
    if (!piece) return false;
    return (toX >= 0 && toX < 8 && toY >= 0 && toY < 8);
}
