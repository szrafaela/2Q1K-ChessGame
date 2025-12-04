#include "Game.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

namespace {
const std::string kSaveFile = "savegame.json";

std::optional<std::pair<int, int>> parseSquare(const std::string& coord) {
    if (coord.size() != 2) return std::nullopt;
    char file = static_cast<char>(std::tolower(static_cast<unsigned char>(coord[0])));
    char rank = coord[1];
    if (file < 'a' || file > 'h') return std::nullopt;
    if (rank < '1' || rank > '8') return std::nullopt;
    int x = file - 'a';
    int y = rank - '1';
    return std::make_pair(x, y);
}

void printBoard(const Game& game) {
    const Board& board = game.getBoard();
    std::cout << "\n   a b c d e f g h\n";
    for (int y = 7; y >= 0; --y) {
        std::cout << (y + 1) << "  ";
        for (int x = 0; x < 8; ++x) {
            auto piece = board.getPieceAt(x, y);
            if (piece) {
                std::cout << piece->getSymbol();
            } else {
                std::cout << '.';
            }
            std::cout << ' ';
        }
        std::cout << " " << (y + 1) << '\n';
    }
    std::cout << "   a b c d e f g h\n";
}

void printHelp() {
    std::cout << "\nAvailable commands:\n"
              << "  move <from> <to>        - e.g. move e2 e4\n"
              << "  undo                    - undo last move\n"
              << "  show                    - print board\n"
              << "  save                    - save game (savegame.json)\n"
              << "  load                    - load game (savegame.json)\n"
              << "  name <white|black> <name> - set player name\n"
              << "  help                    - show this help\n"
              << "  quit                    - exit game\n";
}

PieceType promptPromotionChoice() {
    std::cout << "Choose promotion piece (q = queen, r = rook, b = bishop, n = knight). Default: queen: ";
    std::string input;
    std::getline(std::cin, input);
    if (input.empty()) return PieceType::Queen;
    char c = static_cast<char>(std::tolower(static_cast<unsigned char>(input.front())));
    switch (c) {
        case 'q': return PieceType::Queen;
        case 'r': return PieceType::Rook;
        case 'b': return PieceType::Bishop;
        case 'n': return PieceType::Knight;
        default: return PieceType::Queen;
    }
}
} // namespace

int main() {
    Game game;

    std::ifstream infile(kSaveFile);
    if (infile.good()) {
        std::cout << "Loading previous save..." << std::endl;
        game.loadFromFile(kSaveFile);
    } else {
        std::cout << "Starting new game..." << std::endl;
        game.start();
    }

    std::cout << "Chess game started!" << std::endl;
    printBoard(game);
    printHelp();

    std::string line;
    while (true) {
        std::cout << '\n'
                  << (game.isWhiteTurn() ? game.getPlayerName(Color::White)
                                          : game.getPlayerName(Color::Black))
                  << " to move > ";
        if (!std::getline(std::cin, line)) {
            break;
        }
        std::stringstream ss(line);
        std::string command;
        ss >> command;
        if (command.empty()) {
            continue;
        }

        if (command == "move") {
            std::string from, to;
            ss >> from >> to;
            if (from.empty() || to.empty()) {
                std::cout << "Usage: move <from> <to> (e.g. move e2 e4)";
                continue;
            }
            auto fromCoord = parseSquare(from);
            auto toCoord = parseSquare(to);
            if (!fromCoord || !toCoord) {
                std::cout << "Invalid square. Use a1-h8.";
                continue;
            }

            PieceType promotionChoice = PieceType::Queen;
            auto piece = game.getBoard().getPieceAt(fromCoord->first, fromCoord->second);
            if (piece && piece->getType() == PieceType::Pawn) {
                int promoRank = (piece->getColor() == Color::White) ? 7 : 0;
                if (toCoord->second == promoRank) {
                    promotionChoice = promptPromotionChoice();
                }
            }

            int beforeMoves = game.getMoveCount();
            game.makeMove(fromCoord->first, fromCoord->second, toCoord->first, toCoord->second, promotionChoice);
            if (game.getMoveCount() == beforeMoves) {
                std::cout << "Illegal move.";
            } else {
                std::cout << "Move recorded.";

                if (game.isCheckmate()) {
                    Color winner = game.isWhiteTurn() ? Color::Black : Color::White;
                    std::cout << "\nCheckmate! " << game.getPlayerName(winner) << " wins.\n";
                    printBoard(game);
                    break;
                } else if (game.isStalemate()) {
                    std::cout << "\nStalemate. Draw.\n";
                    printBoard(game);
                    break;
                } else {
                    Color toMove = game.getCurrentPlayer();
                    if (game.isInCheck(toMove)) {
                        std::cout << " Check! " << game.getPlayerName(toMove) << " is in check.";
                    }
                    printBoard(game);
                }
            }
        } else if (command == "undo") {
            if (game.getMoveCount() == 0) {
                std::cout << "No moves to undo.";
                continue;
            }
            game.undoMove();
            std::cout << "Last move undone.";
            printBoard(game);
        } else if (command == "show") {
            printBoard(game);
        } else if (command == "save") {
            game.saveToFile(kSaveFile);
            std::cout << "Saved to: " << kSaveFile;
        } else if (command == "load") {
            std::ifstream check(kSaveFile);
            if (!check.good()) {
                std::cout << "No saved game in " << kSaveFile << ".";
                continue;
            }
            game.loadFromFile(kSaveFile);
            std::cout << "Game loaded.";
            printBoard(game);
        } else if (command == "name") {
            std::string colorStr;
            ss >> colorStr;
            std::string newName;
            std::getline(ss, newName);
            if (colorStr.empty() || newName.empty()) {
                std::cout << "Usage: name <white|black> <name>";
                continue;
            }
            newName.erase(newName.begin(),
                          std::find_if(newName.begin(), newName.end(), [](unsigned char ch) { return !std::isspace(ch); }));
            std::transform(colorStr.begin(), colorStr.end(), colorStr.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (colorStr == "white") {
                game.setPlayerName(Color::White, newName);
                std::cout << "White player is now: " << newName;
            } else if (colorStr == "black") {
                game.setPlayerName(Color::Black, newName);
                std::cout << "Black player is now: " << newName;
            } else {
                std::cout << "Unknown color. Usage: name <white|black> <name>";
            }
        } else if (command == "help") {
            printHelp();
        } else if (command == "quit" || command == "exit") {
            std::cout << "Exiting...";
            break;
        } else {
            std::cout << "Unknown command. Type 'help' for the list.";
        }
    }

    std::cout << "\nSaving game to JSON..." << std::endl;
    game.saveToFile(kSaveFile);

    std::cout << "Save complete. Goodbye!" << std::endl;
    return 0;
}
