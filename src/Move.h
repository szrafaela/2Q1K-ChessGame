#pragma once
#include "Piece.h"

class Move {
public:
    Move(Piece* piece, int fromX, int fromY, int toX, int toY);
    Piece* getPiece() const;
    int getFromX() const;
    int getFromY() const;
    int getToX() const;
    int getToY() const;

private:
    Piece* piece;
    int fromX, fromY, toX, toY;
};