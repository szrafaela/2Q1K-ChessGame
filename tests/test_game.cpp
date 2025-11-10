#include <gtest/gtest.h>
#include <cstdio>
#include "Game.h"
#include "Board.h"
#include "Piece.h"

namespace {
void RemoveFile(const std::string& filename) {
    std::remove(filename.c_str());
}

void ExpectBoardsEqual(const Board& lhs, const Board& rhs) {
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            auto leftPiece = lhs.getPieceAt(x, y);
            auto rightPiece = rhs.getPieceAt(x, y);
            if (!leftPiece || !rightPiece) {
                EXPECT_EQ(static_cast<bool>(leftPiece), static_cast<bool>(rightPiece))
                    << "Mismatch at (" << x << ", " << y << ")";
                continue;
            }
            EXPECT_EQ(leftPiece->getType(), rightPiece->getType())
                << "Type mismatch at (" << x << ", " << y << ")";
            EXPECT_EQ(leftPiece->getColor(), rightPiece->getColor())
                << "Color mismatch at (" << x << ", " << y << ")";
        }
    }
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
    const Board& board = game.getBoard();

    EXPECT_TRUE(game.isWhiteTurn());
    EXPECT_EQ(game.getMoveCount(), 0);

    game.makeMove(0, 1, 0, 2);
    auto whitePawn = board.getPieceAt(0, 2);
    ASSERT_NE(whitePawn, nullptr);
    EXPECT_EQ(whitePawn->getColor(), Color::White);
    EXPECT_EQ(board.getPieceAt(0, 1), nullptr);
    EXPECT_FALSE(game.isWhiteTurn());
    EXPECT_EQ(game.getMoveCount(), 1);

    game.makeMove(0, 6, 0, 5);
    auto blackPawn = board.getPieceAt(0, 5);
    ASSERT_NE(blackPawn, nullptr);
    EXPECT_EQ(blackPawn->getColor(), Color::Black);
    EXPECT_EQ(board.getPieceAt(0, 6), nullptr);
    EXPECT_TRUE(game.isWhiteTurn());
    EXPECT_EQ(game.getMoveCount(), 2);
}

TEST(GameIntegrationTest, SaveAndLoadGameStateTest) {
    Game game;
    game.start();
    const std::string saveFile = "test_save_load_state.json";

    // White moves king forward one square.
    game.makeMove(4, 0, 4, 1);
    EXPECT_FALSE(game.isWhiteTurn());
    EXPECT_EQ(game.getMoveCount(), 1);

    game.saveToFile(saveFile);

    Game loadedGame;
    loadedGame.loadFromFile(saveFile);

    EXPECT_EQ(loadedGame.getMoveCount(), game.getMoveCount());
    EXPECT_EQ(loadedGame.getCurrentPlayer(), game.getCurrentPlayer());
    ExpectBoardsEqual(loadedGame.getBoard(), game.getBoard());

    // Attempt to move a white pawn even though it's black's turn.
    auto whitePawn = loadedGame.getBoard().getPieceAt(0, 1);
    auto targetSquare = loadedGame.getBoard().getPieceAt(0, 2);
    loadedGame.makeMove(0, 1, 0, 2);
    EXPECT_EQ(loadedGame.getMoveCount(), game.getMoveCount());
    EXPECT_EQ(loadedGame.getCurrentPlayer(), game.getCurrentPlayer());
    EXPECT_EQ(loadedGame.getBoard().getPieceAt(0, 1), whitePawn);
    EXPECT_EQ(loadedGame.getBoard().getPieceAt(0, 2), targetSquare);

    // Make a valid black move, then ensure state advanced.
    loadedGame.makeMove(0, 6, 0, 5);
    EXPECT_TRUE(loadedGame.isWhiteTurn());
    EXPECT_EQ(loadedGame.getMoveCount(), game.getMoveCount() + 1);
    auto blackPawn = loadedGame.getBoard().getPieceAt(0, 5);
    ASSERT_NE(blackPawn, nullptr);
    EXPECT_EQ(blackPawn->getColor(), Color::Black);
    EXPECT_EQ(loadedGame.getBoard().getPieceAt(0, 6), nullptr);

    RemoveFile(saveFile);
}

TEST(GameIntegrationTest, UndoMoveTest) {
    Game game;
    game.start();
    auto blockedPawn = game.getBoard().getPieceAt(4, 1);
    ASSERT_NE(blockedPawn, nullptr);

    game.makeMove(4, 0, 4, 1);
    EXPECT_FALSE(game.isWhiteTurn());
    EXPECT_EQ(game.getMoveCount(), 1);
    auto king = game.getBoard().getPieceAt(4, 1);
    ASSERT_NE(king, nullptr);
    EXPECT_EQ(king->getType(), PieceType::King);
    EXPECT_EQ(game.getBoard().getPieceAt(4, 0), nullptr);

    game.undoMove();
    EXPECT_TRUE(game.isWhiteTurn());
    EXPECT_EQ(game.getMoveCount(), 0);
    EXPECT_EQ(game.getBoard().getPieceAt(4, 0), king);
    EXPECT_EQ(king->getX(), 4);
    EXPECT_EQ(king->getY(), 0);
    EXPECT_EQ(game.getBoard().getPieceAt(4, 1), blockedPawn);
}

TEST(GameIntegrationTest, InvalidMoveOutOfBoundsKeepsState) {
    Game game;
    game.start();
    EXPECT_TRUE(game.isWhiteTurn());
    EXPECT_EQ(game.getMoveCount(), 0);

    auto king = game.getBoard().getPieceAt(4, 0);
    ASSERT_NE(king, nullptr);
    EXPECT_EQ(king->getType(), PieceType::King);

    game.makeMove(4, 0, -1, 0);
    EXPECT_TRUE(game.isWhiteTurn());
    EXPECT_EQ(game.getMoveCount(), 0);
    EXPECT_EQ(game.getBoard().getPieceAt(4, 0), king);

    game.makeMove(4, 0, 4, 8);
    EXPECT_TRUE(game.isWhiteTurn());
    EXPECT_EQ(game.getMoveCount(), 0);
    EXPECT_EQ(game.getBoard().getPieceAt(4, 0), king);
}
