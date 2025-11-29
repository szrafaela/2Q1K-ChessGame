#include "Game.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>  // std::setw-hez
#include <cmath>

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

void Game::makeMove(int fromX, int fromY, int toX, int toY, PieceType promotionChoice) {
    auto piece = board.getPieceAt(fromX, fromY);
    if (!piece) return;

    if (piece->getColor() != (whiteTurn ? Color::White : Color::Black))
        return;

    Color moverColor = piece->getColor();
    int dx = toX - fromX;
    int dy = toY - fromY;

    // Castling handling.
    if (piece->getType() == PieceType::King && std::abs(dx) == 2 && dy == 0) {
        bool kingSide = dx > 0;
        if (!canCastle(moverColor, kingSide)) return;

        Move mv(piece.get(), fromX, fromY, toX, toY, nullptr);
        mv.castling = true;
        mv.hadEnPassantTargetBefore = enPassantTarget.has_value();
        if (enPassantTarget) {
            mv.prevEnPassantX = enPassantTarget->first;
            mv.prevEnPassantY = enPassantTarget->second;
        }
        mv.pieceMovedBefore = piece->hasMoved();

        int rookFromX = kingSide ? 7 : 0;
        int rookFromY = (moverColor == Color::White) ? 0 : 7;
        int rookToX = kingSide ? 5 : 3;
        int rookToY = rookFromY;

        auto rook = board.getPieceAt(rookFromX, rookFromY);

        board.movePiece(fromX, fromY, toX, toY);
        board.movePiece(rookFromX, rookFromY, rookToX, rookToY);

        bool leavesInCheck = isInCheck(moverColor);
        if (leavesInCheck) {
            // revert
            board.movePiece(toX, toY, fromX, fromY);
            board.movePiece(rookToX, rookToY, rookFromX, rookFromY);
            return;
        }

        mv.rookFromX = rookFromX;
        mv.rookFromY = rookFromY;
        mv.rookToX = rookToX;
        mv.rookToY = rookToY;

        piece->markMoved();
        if (rook) rook->markMoved();
        enPassantTarget.reset();

        moveHistory.push_back(mv);
        whiteTurn = !whiteTurn;
        currentPlayer = whiteTurn ? Color::White : Color::Black;
        moveCount++;
        return;
    }

    // En passant handling: detect special capture.
    bool isEnPassantCapture = false;
    std::shared_ptr<Piece> capturedPiece = board.getPieceAt(toX, toY);
    int captureX = toX, captureY = toY;

    if (piece->getType() == PieceType::Pawn &&
        std::abs(dx) == 1 &&
        dy == ((moverColor == Color::White) ? 1 : -1) &&
        !capturedPiece && enPassantTarget &&
        enPassantTarget->first == toX && enPassantTarget->second == toY) {
        isEnPassantCapture = true;
        captureX = toX;
        captureY = fromY; // pawn being captured is on the fromY rank
        capturedPiece = board.getPieceAt(captureX, captureY);
        if (!capturedPiece || capturedPiece->getType() != PieceType::Pawn || capturedPiece->getColor() == moverColor) {
            return;
        }
    }

    if (!isEnPassantCapture && !board.isValidMove(piece, toX, toY))
        return;

    Move mv(piece.get(), fromX, fromY, toX, toY, capturedPiece);
    mv.hadEnPassantTargetBefore = enPassantTarget.has_value();
    if (enPassantTarget) {
        mv.prevEnPassantX = enPassantTarget->first;
        mv.prevEnPassantY = enPassantTarget->second;
    }
    mv.pieceMovedBefore = piece->hasMoved();

    // Apply capture for en passant.
    if (isEnPassantCapture) {
        board.setPieceAt(captureX, captureY, nullptr);
        mv.enPassant = true;
        mv.enPassantCapturedX = captureX;
        mv.enPassantCapturedY = captureY;
    }

    // Tentatively make the move.
    board.movePiece(fromX, fromY, toX, toY);

    // Handle promotion (choose piece).
    bool promoted = false;
    if (piece->getType() == PieceType::Pawn &&
        ((moverColor == Color::White && toY == 7) || (moverColor == Color::Black && toY == 0))) {
        PieceType chosen = promotionChoice;
        switch (chosen) {
            case PieceType::Queen:
            case PieceType::Rook:
            case PieceType::Bishop:
            case PieceType::Knight:
                break;
            default:
                chosen = PieceType::Queen;
                break;
        }
        auto promotedPiece = Piece::create(chosen, moverColor, toX, toY);
        promotedPiece->markMoved();
        mv.promotion = true;
        mv.promotedFrom = board.getPieceAt(toX, toY);
        board.setPieceAt(toX, toY, promotedPiece);
        promoted = true;
    }

    // Reject moves that leave own king in check.
    if (isInCheck(moverColor)) {
        // revert promotion
        if (promoted) {
            board.setPieceAt(toX, toY, mv.promotedFrom);
        }
        board.movePiece(toX, toY, fromX, fromY);
        if (capturedPiece) {
            board.setPieceAt(captureX, captureY, capturedPiece);
            capturedPiece->setPosition(captureX, captureY);
        }
        if (mv.enPassant) {
            // already restored by capturedPiece block
        }
        if (mv.hadEnPassantTargetBefore) {
            enPassantTarget = std::make_optional(std::make_pair(mv.prevEnPassantX, mv.prevEnPassantY));
        } else {
            enPassantTarget.reset();
        }
        return;
    }

    piece->markMoved();

    enPassantTarget.reset();
    // set en passant target if pawn double-step
    if (piece->getType() == PieceType::Pawn && std::abs(dy) == 2) {
        int passedY = (fromY + toY) / 2;
        enPassantTarget = std::make_pair(toX, passedY);
    }

    moveHistory.push_back(mv);
    whiteTurn = !whiteTurn;
    currentPlayer = whiteTurn ? Color::White : Color::Black;
    moveCount++;
}

void Game::undoMove() {
    if (moveHistory.empty()) return;
    Move last = moveHistory.back();
    whiteTurn = !whiteTurn;
    currentPlayer = whiteTurn ? Color::White : Color::Black;

    // Restore en passant target prior to the move.
    if (last.hadEnPassantTargetBefore) {
        enPassantTarget = std::make_pair(last.prevEnPassantX, last.prevEnPassantY);
    } else {
        enPassantTarget.reset();
    }

    if (last.castling) {
        board.movePiece(last.getToX(), last.getToY(), last.getFromX(), last.getFromY());
        board.movePiece(last.rookToX, last.rookToY, last.rookFromX, last.rookFromY);
        auto king = board.getPieceAt(last.getFromX(), last.getFromY());
        auto rook = board.getPieceAt(last.rookFromX, last.rookFromY);
        if (king) king->setMoved(last.pieceMovedBefore);
        if (rook) rook->setMoved(false); // once castled they had moved; undo -> not moved
    } else {
        // undo promotion by restoring the original pawn
        if (last.promotion && last.promotedFrom) {
            board.setPieceAt(last.getToX(), last.getToY(), last.promotedFrom);
        }
        board.movePiece(last.getToX(), last.getToY(), last.getFromX(), last.getFromY());

        // Restore captured piece (including en passant).
        if (auto captured = last.getCapturedPiece()) {
            int capX = last.enPassant ? last.enPassantCapturedX : last.getToX();
            int capY = last.enPassant ? last.enPassantCapturedY : last.getToY();
            board.setPieceAt(capX, capY, captured);
            captured->setPosition(capX, capY);
        }

        auto movedPiece = board.getPieceAt(last.getFromX(), last.getFromY());
        if (movedPiece) movedPiece->setMoved(last.pieceMovedBefore);
    }

    moveHistory.pop_back();
    if (moveCount > 0) moveCount--;
}

bool Game::hasLegalMove(Color color) {
    int direction = (color == Color::White) ? 1 : -1;
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            auto piece = board.getPieceAt(x, y);
            if (!piece || piece->getColor() != color) continue;

            // Castling possibilities.
            if (piece->getType() == PieceType::King) {
                if (canCastle(color, true) || canCastle(color, false)) {
                    return true;
                }
            }

            for (int toY = 0; toY < 8; ++toY) {
                for (int toX = 0; toX < 8; ++toX) {
                    // En passant simulation.
                    if (piece->getType() == PieceType::Pawn &&
                        std::abs(toX - x) == 1 &&
                        (toY - y) == direction &&
                        enPassantTarget &&
                        enPassantTarget->first == toX &&
                        enPassantTarget->second == toY &&
                        !board.getPieceAt(toX, toY)) {

                        auto captured = board.getPieceAt(toX, y);
                        if (!captured || captured->getColor() == color) continue;

                        board.setPieceAt(toX, y, nullptr);
                        board.movePiece(x, y, toX, toY);
                        bool leavesInCheck = isInCheck(color);
                        board.movePiece(toX, toY, x, y);
                        board.setPieceAt(toX, y, captured);
                        captured->setPosition(toX, y);

                        if (!leavesInCheck) return true;
                        continue;
                    }

                    if (!board.isValidMove(piece, toX, toY)) continue;

                    auto captured = board.getPieceAt(toX, toY);
                    board.movePiece(x, y, toX, toY);

                    bool promoted = false;
                    std::shared_ptr<Piece> originalMoved = board.getPieceAt(toX, toY);
                    if (piece->getType() == PieceType::Pawn &&
                        ((color == Color::White && toY == 7) || (color == Color::Black && toY == 0))) {
                        auto promotedPiece = Piece::create(PieceType::Queen, color, toX, toY);
                        board.setPieceAt(toX, toY, promotedPiece);
                        promoted = true;
                    }

                    bool leavesInCheck = isInCheck(color);

                    if (promoted) {
                        board.setPieceAt(toX, toY, originalMoved);
                    }
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

bool Game::isSquareAttacked(int x, int y, Color byColor) const {
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            auto piece = board.getPieceAt(col, row);
            if (!piece || piece->getColor() != byColor) continue;

            // Pawns attack differently (and cannot move straight to attack).
            if (piece->getType() == PieceType::Pawn) {
                int dx = x - col;
                int dy = y - row;
                int dir = (byColor == Color::White) ? 1 : -1;
                if (std::abs(dx) == 1 && dy == dir) {
                    return true;
                }
                continue;
            }

            if (board.isValidMove(piece, x, y)) {
                return true;
            }
        }
    }
    return false;
}

bool Game::canCastle(Color color, bool kingSide) const {
    int y = (color == Color::White) ? 0 : 7;
    int kingX = 4;
    int rookX = kingSide ? 7 : 0;
    int rookToX = kingSide ? 5 : 3;
    int kingToX = kingSide ? 6 : 2;

    auto king = board.getPieceAt(kingX, y);
    auto rook = board.getPieceAt(rookX, y);
    if (!king || !rook) return false;
    if (king->getType() != PieceType::King || rook->getType() != PieceType::Rook) return false;
    if (king->hasMoved() || rook->hasMoved()) return false;
    if (isInCheck(color)) return false;

    int start = std::min(kingX, rookX) + 1;
    int end = std::max(kingX, rookX) - 1;
    for (int x = start; x <= end; ++x) {
        if (board.getPieceAt(x, y)) return false;
    }

    // Squares the king crosses must not be attacked.
    Color opponent = (color == Color::White) ? Color::Black : Color::White;
    for (int x : {kingX + (kingSide ? 1 : -1), kingToX}) {
        if (isSquareAttacked(x, y, opponent)) return false;
    }

    return true;
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
    return isSquareAttacked(kingX, kingY, (color == Color::White) ? Color::Black : Color::White);
}

void Game::saveToFile(const std::string& filename) {
    json j;

    j["turn"] = (currentPlayer == Color::White) ? "white" : "black";
    j["move_count"] = moveCount;
    if (enPassantTarget) {
        j["en_passant"] = { {"x", enPassantTarget->first}, {"y", enPassantTarget->second} };
    } else {
        j["en_passant"] = nullptr;
    }

    std::vector<std::vector<std::string>> boardData;
    std::vector<std::vector<bool>> movedData;
    for (int y = 0; y < 8; ++y) {
        std::vector<std::string> row;
        std::vector<bool> movedRow;
        for (int x = 0; x < 8; ++x) {
            auto piece = board.getPieceAt(x, y);
            if (piece) {
                row.push_back(std::string(1, piece->getSymbol()));  // �� char ��' string
                movedRow.push_back(piece->hasMoved());
            } else {
                row.push_back(".");
                movedRow.push_back(false);
            }
        }
        boardData.push_back(row);
        movedData.push_back(movedRow);
    }
    j["board"] = boardData;
    j["moved"] = movedData;

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

    moveHistory.clear();
    std::string turnStr = j["turn"];
    currentPlayer = (turnStr == "white") ? Color::White : Color::Black;
    whiteTurn = (currentPlayer == Color::White);
    moveCount = j["move_count"];
    if (j.contains("en_passant") && !j["en_passant"].is_null()) {
        enPassantTarget = std::make_pair(j["en_passant"]["x"].get<int>(), j["en_passant"]["y"].get<int>());
    } else {
        enPassantTarget.reset();
    }

    auto boardData = j["board"];
    bool hasMovedData = j.contains("moved");
    json movedData;
    if (hasMovedData) {
        movedData = j["moved"];
    }

    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            std::string symbol = boardData[y][x];
            if (symbol != ".") {
                auto piece = Piece::createFromSymbol(symbol[0], x, y);
                if (hasMovedData) {
                    piece->setMoved(movedData[y][x]);
                }
                board.setPieceAt(x, y, piece); // �� char, x, y
            } else {
                board.setPieceAt(x, y, nullptr);
            }
        }
    }
}
