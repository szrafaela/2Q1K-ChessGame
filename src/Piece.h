#pragma once
#include <string>
#include <memory>
#include <cctype>

enum class PieceType { King, Queen, Rook, Bishop, Knight, Pawn };
enum class Color { White, Black };

class Piece {
public:
    Piece(PieceType type, Color color, int x, int y)
        : type(type), color(color), x(x), y(y) {}

    PieceType getType() const { return type; }
    Color getColor() const { return color; }
    int getX() const { return x; }
    int getY() const { return y; }

    void setPosition(int newX, int newY) { x = newX; y = newY; }

    // 🔹 JSON-hoz szükséges segédfüggvények:
    char getSymbol() const {
        char symbol = '?';
        switch (type) {
            case PieceType::Pawn: symbol = 'P'; break;
            case PieceType::Rook: symbol = 'R'; break;
            case PieceType::Knight: symbol = 'N'; break;
            case PieceType::Bishop: symbol = 'B'; break;
            case PieceType::Queen: symbol = 'Q'; break;
            case PieceType::King: symbol = 'K'; break;
        }
        return (color == Color::White) ? std::toupper(symbol) : std::tolower(symbol);
    }

    static std::shared_ptr<Piece> createFromSymbol(char symbol, int x, int y) {
        if (symbol == '.') return nullptr;

        Color color = std::isupper(symbol) ? Color::White : Color::Black;
        char typeChar = std::toupper(symbol);
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

        return std::make_shared<Piece>(type, color, x, y);
    }

private:
    PieceType type;
    Color color;
    int x, y;
};
