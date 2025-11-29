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
    auto piece = board.getPieceAt(fromX, fromY);
    if (!piece) return;

    if (piece->getColor() != (whiteTurn ? Color::White : Color::Black))
        return;

    if (!board.isValidMove(piece, toX, toY))
        return;

    auto capturedPiece = board.getPieceAt(toX, toY);

    // Tentatively make the move.
    board.movePiece(fromX, fromY, toX, toY);

    // Reject moves that leave own king in check.
    if (isInCheck(piece->getColor())) {
        board.movePiece(toX, toY, fromX, fromY);
        if (capturedPiece) {
            board.setPieceAt(toX, toY, capturedPiece);
            capturedPiece->setPosition(toX, toY);
        }
        return;
    }

    moveHistory.emplace_back(piece.get(), fromX, fromY, toX, toY, capturedPiece);
    whiteTurn = !whiteTurn;
    currentPlayer = whiteTurn ? Color::White : Color::Black;
    moveCount++;
}

void Game::undoMove() {
    if (moveHistory.empty()) return;
    Move last = moveHistory.back();
    board.movePiece(last.getToX(), last.getToY(), last.getFromX(), last.getFromY());
    if (auto captured = last.getCapturedPiece()) {
        board.setPieceAt(last.getToX(), last.getToY(), captured);
        captured->setPosition(last.getToX(), last.getToY());
    }
    moveHistory.pop_back();
    whiteTurn = !whiteTurn;
    currentPlayer = whiteTurn ? Color::White : Color::Black;
    if (moveCount > 0) moveCount--;
}

bool Game::hasLegalMove(Color color) {
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            auto piece = board.getPieceAt(x, y);
            if (!piece || piece->getColor() != color) continue;

            for (int toY = 0; toY < 8; ++toY) {
                for (int toX = 0; toX < 8; ++toX) {
                    if (!board.isValidMove(piece, toX, toY)) continue;
                    auto captured = board.getPieceAt(toX, toY);
                    board.movePiece(x, y, toX, toY);
                    bool leavesInCheck = isInCheck(color);
                    board.movePiece(toX, toY, x, y);
                    if (captured) {
                        board.setPieceAt(toX, toY, captured);
                        captured->setPosition(toX, toY);
                    }
                    if (!leavesInCheck) return true;
                }
            }
        }
    }
    return false;
}

bool Game::isCheckmate() {
    Color toMove = currentPlayer;
    if (!isInCheck(toMove)) return false;
    return !hasLegalMove(toMove);
}

bool Game::isStalemate() {
    Color toMove = currentPlayer;
    if (isInCheck(toMove)) return false;
    return !hasLegalMove(toMove);
}

const Board& Game::getBoard() const {
    return board;
}

bool Game::isWhiteTurn() const {
    return whiteTurn;
}

Color Game::getCurrentPlayer() const {
    return currentPlayer;
}

int Game::getMoveCount() const {
    return moveCount;
}

bool Game::isInCheck(Color color) const {
    int kingX = -1, kingY = -1;

    // Locate the king of the given color.
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            auto piece = board.getPieceAt(x, y);
            if (piece && piece->getType() == PieceType::King && piece->getColor() == color) {
                kingX = x;
                kingY = y;
                break;
            }
        }
    }

    if (kingX == -1 || kingY == -1) return false;

    // See if any opposing piece can attack the king's square.
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            auto piece = board.getPieceAt(x, y);
            if (piece && piece->getColor() != color) {
                if (board.isValidMove(piece, kingX, kingY)) {
                    return true;
                }
            }
        }
    }
    return false;
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
                row.push_back(std::string(1, piece->getSymbol()));  // �� char ��' string
            else
                row.push_back(".");
        }
        boardData.push_back(row);
    }
    j["board"] = boardData;

    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Nem sikerƕlt megnyitni a f��jlt ��r��sra: " << filename << "\n";
        return;
    }
    file << std::setw(4) << j;
}

void Game::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Nem sikerƕlt megnyitni a f��jlt: " << filename << "\n";
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
                board.setPieceAt(x, y, Piece::createFromSymbol(symbol[0], x, y)); // �� char, x, y
            } else {
                board.setPieceAt(x, y, nullptr);
            }
        }
    }
}
