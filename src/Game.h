#pragma once
#include <vector>
#include <string>
#include "Board.h"
#include "Player.h"
#include "Move.h"
#include "Piece.h"
#include <optional>
#include <utility>

// A j��t�ck logik��j��t kezel�' oszt��ly
class Game {
public:
    Game();

    void start();
    void makeMove(int fromX, int fromY, int toX, int toY, PieceType promotionChoice = PieceType::Queen);
    void undoMove();
    bool isCheckmate();
    bool isStalemate();
    const Board& getBoard() const;
    bool isWhiteTurn() const;
    Color getCurrentPlayer() const;
    int getMoveCount() const;
    std::string getPlayerName(Color color) const;
    void setPlayerName(Color color, const std::string& name);
    std::optional<std::pair<int, int>> getEnPassantTarget() const;
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
    std::optional<std::pair<int, int>> enPassantTarget;

    bool hasLegalMove(Color color);
    bool canCastle(Color color, bool kingSide) const;
    bool isSquareAttacked(int x, int y, Color byColor) const;
};
