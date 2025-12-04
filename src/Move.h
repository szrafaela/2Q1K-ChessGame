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

public:
    bool castling = false;
    int rookFromX = -1, rookFromY = -1, rookToX = -1, rookToY = -1;
    bool enPassant = false;
    int enPassantCapturedX = -1, enPassantCapturedY = -1;
    bool promotion = false;
    std::shared_ptr<Piece> promotedFrom;
    bool hadEnPassantTargetBefore = false;
    int prevEnPassantX = -1, prevEnPassantY = -1;
    bool pieceMovedBefore = false;
};
