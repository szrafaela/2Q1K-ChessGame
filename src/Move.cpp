#include "Move.h"

Move::Move(Piece* piece, int fromX, int fromY, int toX, int toY)
    : piece(piece), fromX(fromX), fromY(fromY), toX(toX), toY(toY) {}

Piece* Move::getPiece() const { return piece; }
int Move::getFromX() const { return fromX; }
int Move::getFromY() const { return fromY; }
int Move::getToX() const { return toX; }
int Move::getToY() const { return toY; }