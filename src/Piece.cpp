#include "Piece.h"
#include "Board.h"
#include <cmath>

namespace {
int sign(int value) {
    if (value == 0) return 0;
    return (value > 0) ? 1 : -1;
}

// Checks that all intermediate squares between from and to are empty (excludes endpoints).
bool isPathClear(const Board& board, int fromX, int fromY, int toX, int toY) {
    int stepX = sign(toX - fromX);
    int stepY = sign(toY - fromY);
    int x = fromX + stepX;
    int y = fromY + stepY;
    while (x != toX || y != toY) {
        if (board.getPieceAt(x, y)) {
            return false;
        }
        x += stepX;
        y += stepY;
    }
    return true;
}
} // namespace

std::shared_ptr<Piece> Piece::createFromSymbol(char symbol, int x, int y) {
    if (symbol == '.') return nullptr;

    Color color = std::isupper(static_cast<unsigned char>(symbol)) ? Color::White : Color::Black;
    char typeChar = static_cast<char>(std::toupper(static_cast<unsigned char>(symbol)));
    PieceType type;

    switch (typeChar) {
        case 'P': type = PieceType::Pawn; break;
        case 'R': type = PieceType::Rook; break;
        case 'N': type = PieceType::Knight; break;
        case 'B': type = PieceType::Bishop; break;
        case 'Q': type = PieceType::Queen; break;
        case 'K': type = PieceType::King; break;
        default: type = PieceType::Pawn; break;
    }

    return create(type, color, x, y);
}

std::shared_ptr<Piece> Piece::create(PieceType type, Color color, int x, int y) {
    switch (type) {
        case PieceType::King:   return std::make_shared<KingPiece>(color, x, y);
        case PieceType::Queen:  return std::make_shared<QueenPiece>(color, x, y);
        case PieceType::Rook:   return std::make_shared<RookPiece>(color, x, y);
        case PieceType::Bishop: return std::make_shared<BishopPiece>(color, x, y);
        case PieceType::Knight: return std::make_shared<KnightPiece>(color, x, y);
        case PieceType::Pawn:
        default:                return std::make_shared<PawnPiece>(color, x, y);
    }
}

bool Piece::isValidMove(const Board&, int, int) const {
    return false;
}

bool KingPiece::isValidMove(const Board& board, int toX, int toY) const {
    int dx = std::abs(toX - getX());
    int dy = std::abs(toY - getY());
    if (dx == 0 && dy == 0) return false;

    return dx <= 1 && dy <= 1;
}

bool QueenPiece::isValidMove(const Board& board, int toX, int toY) const {
    int dx = std::abs(toX - getX());
    int dy = std::abs(toY - getY());
    if (dx == 0 && dy == 0) return false;

    if (dx == dy || dx == 0 || dy == 0) {
        return isPathClear(board, getX(), getY(), toX, toY);
    }
    return false;
}

bool RookPiece::isValidMove(const Board& board, int toX, int toY) const {
    if (getX() == toX && getY() == toY) return false;

    if (getX() == toX || getY() == toY) {
        return isPathClear(board, getX(), getY(), toX, toY);
    }
    return false;
}

bool BishopPiece::isValidMove(const Board& board, int toX, int toY) const {
    int dx = std::abs(toX - getX());
    int dy = std::abs(toY - getY());
    if (dx == 0 && dy == 0) return false;

    if (dx == dy) {
        return isPathClear(board, getX(), getY(), toX, toY);
    }
    return false;
}

bool KnightPiece::isValidMove(const Board& board, int toX, int toY) const {
    int dx = std::abs(toX - getX());
    int dy = std::abs(toY - getY());
    if (dx == 0 && dy == 0) return false;

    return (dx == 1 && dy == 2) || (dx == 2 && dy == 1);
}

bool PawnPiece::isValidMove(const Board& board, int toX, int toY) const {
    int dx = toX - getX();
    int dy = toY - getY();
    int direction = (getColor() == Color::White) ? 1 : -1;
    auto dest = board.getPieceAt(toX, toY);

    // Forward move
    if (dx == 0) {
        if (dest) return false; // cannot move onto occupied square
        if (dy == direction) return true;
        if (dy == 2 * direction && isStartingRank()) {
            int betweenY = getY() + direction;
            if (!board.getPieceAt(getX(), betweenY)) {
                return true;
            }
        }
    }

    // Diagonal capture
    if (std::abs(dx) == 1 && dy == direction) {
        return dest && dest->getColor() != getColor();
    }

    return false;
}

bool PawnPiece::isStartingRank() const {
    return (getColor() == Color::White && getY() == 1) ||
           (getColor() == Color::Black && getY() == 6);
}
