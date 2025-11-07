#include <gtest/gtest.h>
#include <cstdio>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include "Game.h"
#include "Board.h"
#include "Piece.h"

namespace {
using json = nlohmann::json;

json ReadJson(const std::string& filename) {
    std::ifstream in(filename);
    if (!in.is_open()) {
        throw std::runtime_error("Unable to open JSON file: " + filename);
    }
    json data;
    in >> data;
    return data;
}

json SaveAndRead(Game& game, const std::string& filename) {
    game.saveToFile(filename);
    return ReadJson(filename);
}

void RemoveFile(const std::string& filename) {
    std::remove(filename.c_str());
}
} // namespace

TEST(GameTest, InitializesCorrectly) {
    Game game;
    game.start();
    EXPECT_FALSE(game.isCheckmate());
    EXPECT_FALSE(game.isStalemate());
}

TEST(BoardTest, CanMovePiece) {
    Board board;
    board.initialize();

    // getPieceAt() már std::shared_ptr<Piece>-et ad vissza
    auto king = board.getPieceAt(4, 0);
    ASSERT_NE(king, nullptr);

    board.movePiece(4, 0, 4, 1);
    EXPECT_EQ(king->getY(), 1);
}

TEST(PieceTest, PositionUpdates) {
    Piece piece(PieceType::Knight, Color::White, 1, 0);
    piece.setPosition(2, 2);
    EXPECT_EQ(piece.getX(), 2);
    EXPECT_EQ(piece.getY(), 2);
}

TEST(GameIntegrationTest, GameInitializationTest) {
    Game game;
    game.start();
    const Board& board = game.getBoard();

    auto whiteKing = board.getPieceAt(4, 0);
    ASSERT_NE(whiteKing, nullptr);
    EXPECT_EQ(whiteKing->getType(), PieceType::King);
    EXPECT_EQ(whiteKing->getColor(), Color::White);

    auto blackKing = board.getPieceAt(4, 7);
    ASSERT_NE(blackKing, nullptr);
    EXPECT_EQ(blackKing->getType(), PieceType::King);
    EXPECT_EQ(blackKing->getColor(), Color::Black);

    for (int x = 0; x < 8; ++x) {
        auto whitePawn = board.getPieceAt(x, 1);
        ASSERT_NE(whitePawn, nullptr) << "Missing white pawn at column " << x;
        EXPECT_EQ(whitePawn->getType(), PieceType::Pawn);
        EXPECT_EQ(whitePawn->getColor(), Color::White);

        auto blackPawn = board.getPieceAt(x, 6);
        ASSERT_NE(blackPawn, nullptr) << "Missing black pawn at column " << x;
        EXPECT_EQ(blackPawn->getType(), PieceType::Pawn);
        EXPECT_EQ(blackPawn->getColor(), Color::Black);
    }

    for (int x = 0; x < 8; ++x) {
        if (x != 4) {
            EXPECT_EQ(board.getPieceAt(x, 0), nullptr);
            EXPECT_EQ(board.getPieceAt(x, 7), nullptr);
        }
    }

    for (int y = 2; y <= 5; ++y) {
        for (int x = 0; x < 8; ++x) {
            EXPECT_EQ(board.getPieceAt(x, y), nullptr);
        }
    }
}

TEST(GameIntegrationTest, CompleteMoveCycleTest) {
    Game game;
    game.start();
    const std::string filename = "test_complete_cycle.json";

    auto initialState = SaveAndRead(game, filename);
    EXPECT_EQ(initialState["turn"], "white");
    EXPECT_EQ(initialState["move_count"], 0);

    game.makeMove(0, 1, 0, 2);
    const Board& board = game.getBoard();
    auto whitePawn = board.getPieceAt(0, 2);
    ASSERT_NE(whitePawn, nullptr);
    EXPECT_EQ(whitePawn->getColor(), Color::White);
    EXPECT_EQ(board.getPieceAt(0, 1), nullptr);

    auto afterWhiteMove = SaveAndRead(game, filename);
    EXPECT_EQ(afterWhiteMove["turn"], "black");
    EXPECT_EQ(afterWhiteMove["move_count"], 1);
    EXPECT_EQ(afterWhiteMove["board"][2][0].get<std::string>(), "P");
    EXPECT_EQ(afterWhiteMove["board"][1][0].get<std::string>(), ".");

    game.makeMove(0, 6, 0, 5);
    auto blackPawn = board.getPieceAt(0, 5);
    ASSERT_NE(blackPawn, nullptr);
    EXPECT_EQ(blackPawn->getColor(), Color::Black);
    EXPECT_EQ(board.getPieceAt(0, 6), nullptr);

    auto afterBlackMove = SaveAndRead(game, filename);
    EXPECT_EQ(afterBlackMove["turn"], "white");
    EXPECT_EQ(afterBlackMove["move_count"], 2);
    EXPECT_EQ(afterBlackMove["board"][5][0].get<std::string>(), "p");
    EXPECT_EQ(afterBlackMove["board"][6][0].get<std::string>(), ".");

    RemoveFile(filename);
}

TEST(GameIntegrationTest, SaveAndLoadGameStateTest) {
    Game game;
    game.start();
    const std::string initialSave = "test_save_load_state.json";
    const std::string compareSave = "test_save_load_state_compare.json";
    const std::string invalidSave = "test_save_load_state_invalid.json";
    const std::string continueSave = "test_save_load_state_continue.json";

    auto originalState = SaveAndRead(game, initialSave);
    // White moves king forward one square.
    game.makeMove(4, 0, 4, 1);
    originalState = SaveAndRead(game, initialSave);
    EXPECT_EQ(originalState["turn"], "black");
    EXPECT_EQ(originalState["move_count"], 1);

    Game loadedGame;
    loadedGame.loadFromFile(initialSave);
    auto loadedState = SaveAndRead(loadedGame, compareSave);
    EXPECT_EQ(originalState, loadedState);

    // Attempt to move a white pawn even though it's black's turn.
    loadedGame.makeMove(0, 1, 0, 2);
    auto afterInvalidAttempt = SaveAndRead(loadedGame, invalidSave);
    EXPECT_EQ(originalState, afterInvalidAttempt);

    // Make a valid black move, then ensure state advanced.
    loadedGame.makeMove(0, 6, 0, 5);
    auto afterValidMove = SaveAndRead(loadedGame, continueSave);
    EXPECT_EQ(afterValidMove["turn"], "white");
    EXPECT_EQ(afterValidMove["move_count"], 2);
    EXPECT_EQ(afterValidMove["board"][5][0].get<std::string>(), "p");
    EXPECT_EQ(afterValidMove["board"][6][0].get<std::string>(), ".");

    RemoveFile(initialSave);
    RemoveFile(compareSave);
    RemoveFile(invalidSave);
    RemoveFile(continueSave);
}

TEST(GameIntegrationTest, UndoMoveTest) {
    Game game;
    game.start();
    const std::string filename = "test_undo_state.json";

    game.makeMove(4, 0, 4, 1);
    auto afterMove = SaveAndRead(game, filename);
    EXPECT_EQ(afterMove["turn"], "black");
    EXPECT_EQ(afterMove["move_count"], 1);
    EXPECT_EQ(afterMove["board"][1][4].get<std::string>(), "K");
    EXPECT_EQ(afterMove["board"][0][4].get<std::string>(), ".");

    game.undoMove();
    auto afterUndo = SaveAndRead(game, filename);
    EXPECT_EQ(afterUndo["turn"], "white");
    EXPECT_EQ(afterUndo["move_count"], 0);
    EXPECT_EQ(afterUndo["board"][0][4].get<std::string>(), "K");
    // Known limitation: the pawn that was overwritten is not restored.
    EXPECT_EQ(afterUndo["board"][1][4].get<std::string>(), ".");

    const Board& board = game.getBoard();
    auto king = board.getPieceAt(4, 0);
    ASSERT_NE(king, nullptr);
    EXPECT_EQ(king->getType(), PieceType::King);
    EXPECT_EQ(board.getPieceAt(4, 1), nullptr);

    RemoveFile(filename);
}

TEST(GameIntegrationTest, InvalidMoveOutOfBoundsKeepsState) {
    Game game;
    game.start();
    const std::string filename = "test_invalid_move.json";

    auto initialState = SaveAndRead(game, filename);

    game.makeMove(4, 0, -1, 0);
    auto afterInvalid = SaveAndRead(game, filename);
    EXPECT_EQ(initialState, afterInvalid);

    game.makeMove(4, 0, 4, 8);
    auto afterSecondInvalid = SaveAndRead(game, filename);
    EXPECT_EQ(initialState, afterSecondInvalid);

    auto king = game.getBoard().getPieceAt(4, 0);
    ASSERT_NE(king, nullptr);
    EXPECT_EQ(king->getType(), PieceType::King);

    RemoveFile(filename);
}
