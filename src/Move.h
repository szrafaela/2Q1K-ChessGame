#pragma once
#include <memory>
#include "Piece.h"

class Move {
public:
    Move(Piece* piece, int fromX, int fromY, int toX, int toY, std::shared_ptr<Piece> capturedPiece = nullptr);
    Piece* getPiece() const;
    int getFromX() const;
    int getFromY() const;
    int getToX() const;
    int getToY() const;
    std::shared_ptr<Piece> getCapturedPiece() const;

private:
    Piece* piece;
    int fromX, fromY, toX, toY;
    std::shared_ptr<Piece> capturedPiece;
};
