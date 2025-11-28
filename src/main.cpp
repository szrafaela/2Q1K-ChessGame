#include "Game.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <cctype>

namespace {
const std::string kSaveFile = "savegame.json";

std::optional<std::pair<int, int>> parseSquare(const std::string& coord) {
    if (coord.size() != 2) return std::nullopt;
    char file = std::tolower(coord[0]);
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
    std::cout << "\nElérhető parancsok:\n"
              << "  move <from> <to>  - például: move e2 e4\n"
              << "  undo              - utolsó lépés visszavonása\n"
              << "  show              - tábla kirajzolása\n"
              << "  save              - állás mentése (savegame.json)\n"
              << "  load              - állás betöltése (savegame.json)\n"
              << "  help              - parancsok listája\n"
              << "  quit              - kilépés a játékból\n";
}
} // namespace

int main() {
    Game game;

    std::ifstream infile(kSaveFile);
    if (infile.good()) {
        std::cout << "Korábbi mentés betöltése..." << std::endl;
        game.loadFromFile(kSaveFile);
    } else {
        std::cout << "Új játék indítása..." << std::endl;
        game.start();
    }

    std::cout << "Sakkjáték elindult!" << std::endl;
    printBoard(game);
    printHelp();

    std::string line;
    while (true) {
        std::cout << '\n' << (game.isWhiteTurn() ? "Fehér" : "Fekete") << " következik > ";
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
                std::cout << "Használat: move <from> <to> (pl. move e2 e4)";
                continue;
            }
            auto fromCoord = parseSquare(from);
            auto toCoord = parseSquare(to);
            if (!fromCoord || !toCoord) {
                std::cout << "Érvénytelen mező. Engedélyezett formátum: a1-h8.";
                continue;
            }
            int beforeMoves = game.getMoveCount();
            game.makeMove(fromCoord->first, fromCoord->second, toCoord->first, toCoord->second);
            if (game.getMoveCount() == beforeMoves) {
                std::cout << "A lépés érvénytelen.";
            } else {
                std::cout << "Lépés rögzítve.";
                printBoard(game);
            }
        } else if (command == "undo") {
            if (game.getMoveCount() == 0) {
                std::cout << "Nincs visszavonható lépés.";
                continue;
            }
            game.undoMove();
            std::cout << "Utolsó lépés visszavonva.";
            printBoard(game);
        } else if (command == "show") {
            printBoard(game);
        } else if (command == "save") {
            game.saveToFile(kSaveFile);
            std::cout << "Állás elmentve: " << kSaveFile;
        } else if (command == "load") {
            std::ifstream check(kSaveFile);
            if (!check.good()) {
                std::cout << "Nincs mentett állás a " << kSaveFile << " fájlban.";
                continue;
            }
            game.loadFromFile(kSaveFile);
            std::cout << "Állás betöltve.";
            printBoard(game);
        } else if (command == "help") {
            printHelp();
        } else if (command == "quit" || command == "exit") {
            std::cout << "Kilépés...";
            break;
        } else {
            std::cout << "Ismeretlen parancs. Írd be, hogy 'help' a lista megjelenítéséhez.";
        }
    }

    std::cout << "\nJáték mentése JSON fájlba..." << std::endl;
    game.saveToFile(kSaveFile);

    std::cout << "Mentés kész. Viszlát!" << std::endl;
    return 0;
}
