#include "Game.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

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

std::string resolveEnginePath() {
    // Try paths relative to the executable working directory and build directory.
    std::vector<std::string> candidates = {
        // Windows bundled engine
        "external\\stockfish\\stockfish-windows-x86-64-avx2.exe",
        "..\\external\\stockfish\\stockfish-windows-x86-64-avx2.exe"
#ifndef _WIN32
        ,
        // Linux package locations and PATH
        "/usr/games/stockfish",
        "/usr/bin/stockfish",
        "/usr/local/bin/stockfish",
        "stockfish"
#endif
    };
    for (const auto& p : candidates) {
        if (std::filesystem::exists(p)) return p;
    }
    return candidates.front(); // fallback default path
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
              << "  save                    - save game\n"
              << "  load                    - load game\n"
              << "  name <white|black> <name> - set player name\n"
              << "  stockfish                - play vs Stockfish\n"
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

class StockfishProcess {
public:
    bool start(const std::string& path, int skillLevel) {
#ifdef _WIN32
        SECURITY_ATTRIBUTES saAttr{};
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = nullptr;

        if (!CreatePipe(&childStdoutRd, &childStdoutWr, &saAttr, 0)) return false;
        if (!SetHandleInformation(childStdoutRd, HANDLE_FLAG_INHERIT, 0)) return false;
        if (!CreatePipe(&childStdinRd, &childStdinWr, &saAttr, 0)) return false;
        if (!SetHandleInformation(childStdinWr, HANDLE_FLAG_INHERIT, 0)) return false;

        STARTUPINFOA si{};
        si.cb = sizeof(STARTUPINFOA);
        si.hStdError = childStdoutWr;
        si.hStdOutput = childStdoutWr;
        si.hStdInput = childStdinRd;
        si.dwFlags |= STARTF_USESTDHANDLES;

        std::string cmd = "\"" + path + "\"";
        BOOL success = CreateProcessA(
            nullptr,
            cmd.data(),
            nullptr,
            nullptr,
            TRUE,
            0,
            nullptr,
            nullptr,
            &si,
            &pi);

        if (!success) return false;
        running = true;
        childStdoutRdValid = true;
        childStdoutWrValid = true;
        childStdinRdValid = true;
        childStdinWrValid = true;
#else
        int stdinPipe[2];
        int stdoutPipe[2];
        if (pipe(stdinPipe) != 0) return false;
        if (pipe(stdoutPipe) != 0) {
            close(stdinPipe[0]);
            close(stdinPipe[1]);
            return false;
        }
        pid_t pid = fork();
        if (pid == 0) {
            dup2(stdinPipe[0], STDIN_FILENO);
            dup2(stdoutPipe[1], STDOUT_FILENO);
            dup2(stdoutPipe[1], STDERR_FILENO);
            close(stdinPipe[0]);
            close(stdinPipe[1]);
            close(stdoutPipe[0]);
            close(stdoutPipe[1]);
            execl(path.c_str(), path.c_str(), static_cast<char*>(nullptr));
            _exit(1);
        } else if (pid < 0) {
            close(stdinPipe[0]);
            close(stdinPipe[1]);
            close(stdoutPipe[0]);
            close(stdoutPipe[1]);
            return false;
        }

        childPid = pid;
        childStdinFd = stdinPipe[1];
        childStdoutFd = stdoutPipe[0];
        close(stdinPipe[0]);
        close(stdoutPipe[1]);
        running = true;
#endif
        send("uci\n");
        send("setoption name Skill Level value " + std::to_string(skillLevel) + "\n");
        send("isready\n");
        waitFor("readyok");
        return true;
    }

    bool isRunning() const { return running; }

    void stop() {
        if (!running) return;
        send("quit\n");
#ifdef _WIN32
        if (childStdoutWrValid) CloseHandle(childStdoutWr);
        if (childStdinRdValid) CloseHandle(childStdinRd);
        if (childStdinWrValid) CloseHandle(childStdinWr);
        if (childStdoutRdValid) CloseHandle(childStdoutRd);
        WaitForSingleObject(pi.hProcess, 1000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        childStdoutWrValid = childStdinRdValid = childStdinWrValid = childStdoutRdValid = false;
#else
        close(childStdinFd);
        int status = 0;
        waitpid(childPid, &status, 0);
        close(childStdoutFd);
        childStdinFd = -1;
        childStdoutFd = -1;
        childPid = -1;
#endif
        running = false;
    }

    bool send(const std::string& msg) const {
        if (!running) return false;
#ifdef _WIN32
        DWORD written = 0;
        return WriteFile(childStdinWr, msg.c_str(), static_cast<DWORD>(msg.size()), &written, nullptr);
#else
        ssize_t written = write(childStdinFd, msg.c_str(), msg.size());
        return written == static_cast<ssize_t>(msg.size());
#endif
    }

    std::string readLine() const {
        std::string line;
        char ch = 0;
#ifdef _WIN32
        DWORD read = 0;
        while (true) {
            if (!ReadFile(childStdoutRd, &ch, 1, &read, nullptr) || read == 0) break;
            if (ch == '\r') continue;
            if (ch == '\n') break;
            line.push_back(ch);
        }
#else
        ssize_t readBytes = 0;
        while (true) {
            readBytes = read(childStdoutFd, &ch, 1);
            if (readBytes <= 0) break;
            if (ch == '\r') continue;
            if (ch == '\n') break;
            line.push_back(ch);
        }
#endif
        return line;
    }

    std::string waitFor(const std::string& prefix) const {
        std::string line;
        do {
            line = readLine();
        } while (running && line.rfind(prefix, 0) != 0);
        return line;
    }

    std::string bestMove(const std::vector<std::string>& uciMoves, int movetimeMs) {
        if (!running) return "";
        std::string posCmd = "position startpos";
        if (!uciMoves.empty()) {
            posCmd += " moves";
            for (const auto& mv : uciMoves) {
                posCmd += " " + mv;
            }
        }
        posCmd += "\n";
        send(posCmd);
        send("go movetime " + std::to_string(movetimeMs) + "\n");
        auto line = waitFor("bestmove");
        if (line.empty()) return "";
        auto partsPos = line.find(' ');
        if (partsPos == std::string::npos) return "";
        std::string moveStr = line.substr(partsPos + 1);
        auto space = moveStr.find(' ');
        if (space != std::string::npos) moveStr = moveStr.substr(0, space);
        return moveStr;
    }

private:
#ifdef _WIN32
    HANDLE childStdoutRd = nullptr;
    HANDLE childStdoutWr = nullptr;
    HANDLE childStdinRd = nullptr;
    HANDLE childStdinWr = nullptr;
    PROCESS_INFORMATION pi{};
    bool childStdoutRdValid = false;
    bool childStdoutWrValid = false;
    bool childStdinRdValid = false;
    bool childStdinWrValid = false;
#else
    int childStdoutFd = -1;
    int childStdinFd = -1;
    pid_t childPid = -1;
#endif
    bool running = false;
};

std::string coordsToUci(int fromX, int fromY, int toX, int toY, std::optional<PieceType> promo = std::nullopt) {
    auto toFile = [](int x) { return static_cast<char>('a' + x); };
    auto toRank = [](int y) { return static_cast<char>('1' + y); };
    std::string m;
    m.push_back(toFile(fromX));
    m.push_back(toRank(fromY));
    m.push_back(toFile(toX));
    m.push_back(toRank(toY));
    if (promo.has_value()) {
        char c = 'q';
        switch (*promo) {
            case PieceType::Queen: c = 'q'; break;
            case PieceType::Rook: c = 'r'; break;
            case PieceType::Bishop: c = 'b'; break;
            case PieceType::Knight: c = 'n'; break;
            default: break;
        }
        m.push_back(c);
    }
    return m;
}

bool parseUciMove(const std::string& mv, int& fromX, int& fromY, int& toX, int& toY, PieceType& promoType, bool& hasPromo) {
    if (mv.size() < 4) return false;
    fromX = mv[0] - 'a';
    fromY = mv[1] - '1';
    toX = mv[2] - 'a';
    toY = mv[3] - '1';
    hasPromo = false;
    if (mv.size() >= 5) {
        hasPromo = true;
        char p = static_cast<char>(std::tolower(static_cast<unsigned char>(mv[4])));
        switch (p) {
            case 'q': promoType = PieceType::Queen; break;
            case 'r': promoType = PieceType::Rook; break;
            case 'b': promoType = PieceType::Bishop; break;
            case 'n': promoType = PieceType::Knight; break;
            default: hasPromo = false; break;
        }
    }
    return true;
}

void applyEngineMove(Game& game, const std::string& mv) {
    int fx, fy, tx, ty;
    PieceType promo = PieceType::Queen;
    bool hasPromo = false;
    if (!parseUciMove(mv, fx, fy, tx, ty, promo, hasPromo)) return;
    game.makeMove(fx, fy, tx, ty, hasPromo ? promo : PieceType::Queen);
}

} // namespace

int main() {
    Game game;
    StockfishProcess engine;
    bool engineEnabled = false;
    Color engineColor = Color::Black;
    int engineMovetimeMs = 1000;
    std::vector<std::string> uciMoves;

    auto restartGame = [&](const std::string& message) {
        std::cout << message
                  << "\nGame over. Starting a new game. Type 'quit' to exit if you are done.\n";
        game.start();
        uciMoves.clear();
        if (engine.isRunning()) {
            engine.send("ucinewgame\n");
        }
        printBoard(game);
    };

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
                uciMoves.push_back(coordsToUci(fromCoord->first, fromCoord->second, toCoord->first, toCoord->second,
                                               (promotionChoice != PieceType::Queen) ? std::optional<PieceType>(promotionChoice) : std::nullopt));
                std::cout << "Move recorded.";

                if (game.isCheckmate()) {
                    Color winner = game.isWhiteTurn() ? Color::Black : Color::White;
                    std::cout << "\nCheckmate! " << game.getPlayerName(winner) << " wins.\n";
                    restartGame("Checkmate reached.");
                    continue;
                } else if (game.isStalemate()) {
                    std::cout << "\nStalemate. Draw.\n";
                    restartGame("Stalemate reached.");
                    continue;
                } else {
                    Color toMove = game.getCurrentPlayer();
                    if (game.isInCheck(toMove)) {
                        std::cout << " Check! " << game.getPlayerName(toMove) << " is in check.";
                    }
                    printBoard(game);

                    if (engineEnabled && game.getCurrentPlayer() == engineColor && engine.isRunning()) {
                        std::cout << "\nEngine thinking..." << std::endl;
                        std::string best = engine.bestMove(uciMoves, engineMovetimeMs);
                        if (!best.empty()) {
                            uciMoves.push_back(best);
                            int before = game.getMoveCount();
                            applyEngineMove(game, best);
                            if (game.getMoveCount() == before) {
                                std::cout << "Engine move was illegal; skipping.\n";
                            } else {
                                std::cout << "Engine played: " << best << "\n";
                                if (game.isCheckmate()) {
                                    Color winner = game.isWhiteTurn() ? Color::Black : Color::White;
                                    std::cout << "Checkmate! " << game.getPlayerName(winner) << " wins.\n";
                                    restartGame("Checkmate reached.");
                                    continue;
                                } else if (game.isStalemate()) {
                                    std::cout << "Stalemate. Draw.\n";
                                    restartGame("Stalemate reached.");
                                    continue;
                                } else {
                                    Color tm = game.getCurrentPlayer();
                                    if (game.isInCheck(tm)) {
                                        std::cout << game.getPlayerName(tm) << " is in check.\n";
                                    }
                                    printBoard(game);
                                }
                            }
                        } else {
                            std::cout << "Engine failed to return a move.\n";
                        }
                    }
                }
            }
        } else if (command == "undo") {
            if (game.getMoveCount() == 0) {
                std::cout << "No moves to undo.";
                continue;
            }
            game.undoMove();
            if (!uciMoves.empty()) uciMoves.pop_back();
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
        } else if (command == "stockfish") {
            std::cout << "Set skill level (0-20). Default: 10: ";
            std::string skillStr;
            std::getline(std::cin, skillStr);
            int skill = 10;
            if (!skillStr.empty()) {
                try { skill = std::stoi(skillStr); } catch (...) { skill = 10; }
            }
            if (skill < 0) skill = 0;
            if (skill > 20) skill = 20;

            std::cout << "Choose side for engine (white/black). Default: black: ";
            std::string side;
            std::getline(std::cin, side);
            if (side.empty()) side = "black";
            std::transform(side.begin(), side.end(), side.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            engineColor = (side == "white") ? Color::White : Color::Black;

            std::string enginePath = resolveEnginePath();
            if (!std::filesystem::exists(enginePath)) {
                std::cout << "Default engine path not found. Enter path to Stockfish executable: ";
                std::getline(std::cin, enginePath);
            } else {
                std::cout << "Using engine at: " << enginePath << "\n";
            }

            engine.stop();
            uciMoves.clear();
            if (!engine.start(enginePath, skill)) {
                std::cout << "Failed to start engine at: " << enginePath;
            } else {
                engineEnabled = true;
                std::cout << "Engine started as " << (engineColor == Color::White ? "White" : "Black")
                          << " with skill " << skill << ".";
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
    engine.stop();
    return 0;
}
