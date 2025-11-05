#include <gtest/gtest.h>
#include "Game.h"
#include "Board.h"
#include "Piece.h"

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
