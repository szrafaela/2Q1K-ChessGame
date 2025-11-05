#pragma once
#include <vector>
#include <string>
#include "Board.h"
#include "Player.h"
#include "Move.h"
#include "Piece.h"

// A játék logikáját kezelő osztály
class Game {
public:
    Game();

    void start();
    void makeMove(int fromX, int fromY, int toX, int toY);
    void undoMove();
    bool isCheckmate() const;
    bool isStalemate() const;

    // JSON mentés/betöltés
    void saveToFile(const std::string& filename);
    void loadFromFile(const std::string& filename);

private:
    Board board;
    Player white;
    Player black;
    std::vector<Move> moveHistory;

    bool whiteTurn;           // fehér van-e soron
    Color currentPlayer;      // aktuális játékos színe
    int moveCount;            // hány lépés történt eddig
};
