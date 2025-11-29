#pragma once
#include <vector>
#include <string>
#include "Board.h"
#include "Player.h"
#include "Move.h"
#include "Piece.h"

// A j��t�ck logik��j��t kezel�' oszt��ly
class Game {
public:
    Game();

    void start();
    void makeMove(int fromX, int fromY, int toX, int toY);
    void undoMove();
    bool isCheckmate();
    bool isStalemate();
    const Board& getBoard() const;
    bool isWhiteTurn() const;
    Color getCurrentPlayer() const;
    int getMoveCount() const;
    bool isInCheck(Color color) const;

    // JSON ment�cs/bet�lt�cs
    void saveToFile(const std::string& filename);
    void loadFromFile(const std::string& filename);

private:
    Board board;
    Player white;
    Player black;
    std::vector<Move> moveHistory;

    bool whiteTurn;           // feh�cr van-e soron
    Color currentPlayer;      // aktu��lis j��t�ckos sz��ne
    int moveCount;            // h��ny l�cp�cs t�rt�cnt eddig

    bool hasLegalMove(Color color);
};
