#pragma once
#include <vector>
#include <memory>
#include "Piece.h"

class Board {
public:
    Board();

    void initialize();

    std::shared_ptr<Piece> getPieceAt(int x, int y) const;
    void setPieceAt(int x, int y, std::shared_ptr<Piece> piece);
    void movePiece(int fromX, int fromY, int toX, int toY);
    bool isValidMove(std::shared_ptr<Piece> piece, int toX, int toY) const;

private:
    std::vector<std::vector<std::shared_ptr<Piece>>> board;
    bool isInsideBoard(int x, int y) const;
};
