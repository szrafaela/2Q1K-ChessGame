#pragma once
#include <string>
#include <memory>
#include <cctype>

class Board;

enum class PieceType { King, Queen, Rook, Bishop, Knight, Pawn };
enum class Color { White, Black };

class Piece {
public:
    Piece(PieceType type, Color color, int x, int y)
        : type(type), color(color), x(x), y(y) {}

    virtual ~Piece() = default;

    PieceType getType() const { return type; }
    Color getColor() const { return color; }
    int getX() const { return x; }
    int getY() const { return y; }

    void setPosition(int newX, int newY) { x = newX; y = newY; }

    // Symbol helpers for serialization / display.
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

    static std::shared_ptr<Piece> createFromSymbol(char symbol, int x, int y);
    static std::shared_ptr<Piece> create(PieceType type, Color color, int x, int y);

    // Piece-specific movement validation (ignores turn handling).
    virtual bool isValidMove(const Board& board, int toX, int toY) const;

protected:
    PieceType type;
    Color color;
    int x, y;
};

class KingPiece : public Piece {
public:
    KingPiece(Color color, int x, int y) : Piece(PieceType::King, color, x, y) {}
    bool isValidMove(const Board& board, int toX, int toY) const override;
};

class QueenPiece : public Piece {
public:
    QueenPiece(Color color, int x, int y) : Piece(PieceType::Queen, color, x, y) {}
    bool isValidMove(const Board& board, int toX, int toY) const override;
};

class RookPiece : public Piece {
public:
    RookPiece(Color color, int x, int y) : Piece(PieceType::Rook, color, x, y) {}
    bool isValidMove(const Board& board, int toX, int toY) const override;
};

class BishopPiece : public Piece {
public:
    BishopPiece(Color color, int x, int y) : Piece(PieceType::Bishop, color, x, y) {}
    bool isValidMove(const Board& board, int toX, int toY) const override;
};

class KnightPiece : public Piece {
public:
    KnightPiece(Color color, int x, int y) : Piece(PieceType::Knight, color, x, y) {}
    bool isValidMove(const Board& board, int toX, int toY) const override;
};

class PawnPiece : public Piece {
public:
    PawnPiece(Color color, int x, int y) : Piece(PieceType::Pawn, color, x, y) {}
    bool isValidMove(const Board& board, int toX, int toY) const override;

private:
    bool isStartingRank() const;
};
