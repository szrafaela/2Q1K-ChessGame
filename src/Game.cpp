#include "Game.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>  // std::setw-hez

using json = nlohmann::json;

Game::Game()
    : white("White", Color::White),
      black("Black", Color::Black),
      whiteTurn(true),
      moveCount(0),
      currentPlayer(Color::White) {}

void Game::start() {
    board.initialize();
}

void Game::makeMove(int fromX, int fromY, int toX, int toY) {
    // std::shared_ptr<Piece> használata
    auto piece = board.getPieceAt(fromX, fromY);
    if (!piece) return;

    if (piece->getColor() != (whiteTurn ? Color::White : Color::Black))
        return;

    // ideiglenes stub, amíg az isValidMove nincs implementálva
    if (board.isValidMove(piece, toX, toY)) {
        moveHistory.emplace_back(piece.get(), fromX, fromY, toX, toY);
        board.movePiece(fromX, fromY, toX, toY);
        whiteTurn = !whiteTurn;
        currentPlayer = whiteTurn ? Color::White : Color::Black;
        moveCount++;
    }
}

void Game::undoMove() {
    if (moveHistory.empty()) return;
    Move last = moveHistory.back();
    board.movePiece(last.getToX(), last.getToY(), last.getFromX(), last.getFromY());
    moveHistory.pop_back();
    whiteTurn = !whiteTurn;
    currentPlayer = whiteTurn ? Color::White : Color::Black;
    if (moveCount > 0) moveCount--;
}

bool Game::isCheckmate() const {
    return false; // későbbre hagyva
}

bool Game::isStalemate() const {
    return false; // későbbre hagyva
}

const Board& Game::getBoard() const {
    return board;
}

void Game::saveToFile(const std::string& filename) {
    json j;

    j["turn"] = (currentPlayer == Color::White) ? "white" : "black";
    j["move_count"] = moveCount;

    std::vector<std::vector<std::string>> boardData;
    for (int y = 0; y < 8; ++y) {
        std::vector<std::string> row;
        for (int x = 0; x < 8; ++x) {
            auto piece = board.getPieceAt(x, y);
            if (piece)
                row.push_back(std::string(1, piece->getSymbol()));  // ✅ char → string
            else
                row.push_back(".");
        }
        boardData.push_back(row);
    }
    j["board"] = boardData;

    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Nem sikerült megnyitni a fájlt írásra: " << filename << "\n";
        return;
    }
    file << std::setw(4) << j;
}

void Game::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Nem sikerült megnyitni a fájlt: " << filename << "\n";
        return;
    }

    json j;
    file >> j;

    std::string turnStr = j["turn"];
    currentPlayer = (turnStr == "white") ? Color::White : Color::Black;
    whiteTurn = (currentPlayer == Color::White);
    moveCount = j["move_count"];

    auto boardData = j["board"];
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            std::string symbol = boardData[y][x];
            if (symbol != ".") {
                board.setPieceAt(x, y, Piece::createFromSymbol(symbol[0], x, y)); // ✅ char, x, y
            } else {
                board.setPieceAt(x, y, nullptr);
            }
        }
    }
}
