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

    // Full starting setup.
    board[0][0] = Piece::create(PieceType::Rook, Color::White, 0, 0);
    board[0][1] = Piece::create(PieceType::Knight, Color::White, 1, 0);
    board[0][2] = Piece::create(PieceType::Bishop, Color::White, 2, 0);
    board[0][3] = Piece::create(PieceType::Queen, Color::White, 3, 0);
    board[0][4] = Piece::create(PieceType::King, Color::White, 4, 0);
    board[0][5] = Piece::create(PieceType::Bishop, Color::White, 5, 0);
    board[0][6] = Piece::create(PieceType::Knight, Color::White, 6, 0);
    board[0][7] = Piece::create(PieceType::Rook, Color::White, 7, 0);

    board[7][0] = Piece::create(PieceType::Rook, Color::Black, 0, 7);
    board[7][1] = Piece::create(PieceType::Knight, Color::Black, 1, 7);
    board[7][2] = Piece::create(PieceType::Bishop, Color::Black, 2, 7);
    board[7][3] = Piece::create(PieceType::Queen, Color::Black, 3, 7);
    board[7][4] = Piece::create(PieceType::King, Color::Black, 4, 7);
    board[7][5] = Piece::create(PieceType::Bishop, Color::Black, 5, 7);
    board[7][6] = Piece::create(PieceType::Knight, Color::Black, 6, 7);
    board[7][7] = Piece::create(PieceType::Rook, Color::Black, 7, 7);

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
