#include <gtest/gtest.h>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

#include "Game.h"
#include "Board.h"
#include "Piece.h"

namespace {
void RemoveFile(const std::string& filename) {
    std::remove(filename.c_str());
}

void ClearBoard(Board& board) {
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x)
            board.setPieceAt(x, y, nullptr);
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
            EXPECT_EQ(leftPiece->hasMoved(), rightPiece->hasMoved())
                << "Moved flag mismatch at (" << x << ", " << y << ")";
        }
    }
}

std::string WritePositionToTempFile(const nlohmann::json& j) {
    std::filesystem::path path = std::filesystem::temp_directory_path() / std::filesystem::path("chess_test_pos.json");
    std::ofstream out(path.string());
    out << j.dump(2);
    return path.string();
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
    auto pawn = board.getPieceAt(0, 1);
    ASSERT_NE(pawn, nullptr);

    board.movePiece(0, 1, 0, 2);
    EXPECT_EQ(pawn->getY(), 2);
}

TEST(PieceTest, PositionUpdates) {
    Piece piece(PieceType::Knight, Color::White, 1, 0);
    piece.setPosition(2, 2);
    EXPECT_EQ(piece.getX(), 2);
    EXPECT_EQ(piece.getY(), 2);
}

TEST(BoardTest, SlidingPiecesCannotJump) {
    Board b;
    ClearBoard(b);

    auto rook = Piece::create(PieceType::Rook, Color::White, 0, 0);
    auto blocker = Piece::create(PieceType::Pawn, Color::White, 0, 3);
    b.setPieceAt(0, 0, rook);
    b.setPieceAt(0, 3, blocker);

    EXPECT_FALSE(b.isValidMove(rook, 0, 5)); // blocked by pawn on a4
    EXPECT_TRUE(b.isValidMove(rook, 0, 2));  // clear to a3

    auto bishop = Piece::create(PieceType::Bishop, Color::White, 2, 0); // c1
    auto diagBlocker = Piece::create(PieceType::Pawn, Color::White, 4, 2); // e3
    b.setPieceAt(2, 0, bishop);
    b.setPieceAt(4, 2, diagBlocker);

    EXPECT_FALSE(b.isValidMove(bishop, 5, 3)); // c1 -> f4 blocked
    EXPECT_TRUE(b.isValidMove(bishop, 1, 1));  // c1 -> b2 clear
}

TEST(BoardTest, PawnDoubleStepBlocking) {
    Board b;
    ClearBoard(b);
    auto pawn = Piece::create(PieceType::Pawn, Color::White, 4, 1); // e2
    b.setPieceAt(4, 1, pawn);

    EXPECT_EQ(b.getPieceAt(4, 2), nullptr);
    EXPECT_EQ(b.getPieceAt(4, 3), nullptr);
    EXPECT_TRUE(b.isValidMove(pawn, 4, 3));

    auto blocker = Piece::create(PieceType::Pawn, Color::White, 4, 2); // e3
    b.setPieceAt(4, 2, blocker);

    EXPECT_FALSE(b.isValidMove(pawn, 4, 3)); // cannot jump over blocker
    EXPECT_FALSE(b.isValidMove(pawn, 4, 2)); // cannot capture own piece
}

TEST(GameIntegrationTest, GameInitializationTest) {
    Game game;
    game.start();
    const Board& board = game.getBoard();

    const PieceType backRank[8] = {
        PieceType::Rook, PieceType::Knight, PieceType::Bishop, PieceType::Queen,
        PieceType::King, PieceType::Bishop, PieceType::Knight, PieceType::Rook};

    for (int x = 0; x < 8; ++x) {
        auto whitePiece = board.getPieceAt(x, 0);
        ASSERT_NE(whitePiece, nullptr) << "Missing white back-rank piece at column " << x;
        EXPECT_EQ(whitePiece->getType(), backRank[x]);
        EXPECT_EQ(whitePiece->getColor(), Color::White);

        auto blackPiece = board.getPieceAt(x, 7);
        ASSERT_NE(blackPiece, nullptr) << "Missing black back-rank piece at column " << x;
        EXPECT_EQ(blackPiece->getType(), backRank[x]);
        EXPECT_EQ(blackPiece->getColor(), Color::Black);
    }

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

    // White moves a pawn forward one square.
    game.makeMove(4, 1, 4, 2);
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
    auto movedPawn = game.getBoard().getPieceAt(4, 1);
    ASSERT_NE(movedPawn, nullptr);

    // Legal double-step pawn move.
    game.makeMove(4, 1, 4, 3);
    EXPECT_FALSE(game.isWhiteTurn());
    EXPECT_EQ(game.getMoveCount(), 1);
    auto pawn = game.getBoard().getPieceAt(4, 3);
    ASSERT_NE(pawn, nullptr);
    EXPECT_EQ(pawn->getType(), PieceType::Pawn);
    EXPECT_EQ(game.getBoard().getPieceAt(4, 1), nullptr);

    game.undoMove();
    EXPECT_TRUE(game.isWhiteTurn());
    EXPECT_EQ(game.getMoveCount(), 0);
    EXPECT_EQ(game.getBoard().getPieceAt(4, 1), pawn);
    EXPECT_EQ(pawn->getX(), 4);
    EXPECT_EQ(pawn->getY(), 1);
    EXPECT_EQ(game.getBoard().getPieceAt(4, 3), nullptr);
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

TEST(GameIntegrationTest, CastlingKingSideAllowed) {
    Game g;
    g.start();
    int before = g.getMoveCount();
    g.makeMove(4, 1, 4, 3); // e4
    g.makeMove(4, 6, 4, 4); // ...e5
    g.makeMove(6, 0, 5, 2); // Nf3
    g.makeMove(1, 7, 2, 5); // ...Nc6
    g.makeMove(5, 0, 2, 3); // Bc4
    g.makeMove(6, 7, 5, 5); // ...Nf6
    EXPECT_EQ(g.getMoveCount(), before + 6);

    int castleBefore = g.getMoveCount();
    g.makeMove(4, 0, 6, 0); // O-O
    EXPECT_EQ(g.getMoveCount(), castleBefore + 1);

    auto king = g.getBoard().getPieceAt(6, 0);
    auto rook = g.getBoard().getPieceAt(5, 0);
    EXPECT_NE(king, nullptr);
    EXPECT_NE(rook, nullptr);
    EXPECT_EQ(king->getType(), PieceType::King);
    EXPECT_EQ(rook->getType(), PieceType::Rook);
    EXPECT_EQ(g.getBoard().getPieceAt(4, 0), nullptr);
    EXPECT_EQ(g.getBoard().getPieceAt(7, 0), nullptr);
    EXPECT_TRUE(king->hasMoved());
    EXPECT_TRUE(rook->hasMoved());
}

TEST(GameIntegrationTest, CastlingThroughCheckIsRejected) {
    nlohmann::json j;
    j["turn"] = "white";
    j["move_count"] = 0;
    j["white_name"] = "White";
    j["black_name"] = "Black";
    j["en_passant"] = nullptr;
    std::vector<std::vector<std::string>> board(8, std::vector<std::string>(8, "."));
    board[0][4] = "K"; // e1
    board[0][7] = "R"; // h1
    board[7][4] = "k"; // e8
    board[4][2] = "b"; // bishop on c5 attacking g1 through clear squares
    j["board"] = board;
    auto path = WritePositionToTempFile(j);

    Game g;
    g.loadFromFile(path);

    int before = g.getMoveCount();
    g.makeMove(4, 0, 6, 0); // attempt O-O through check on g1
    EXPECT_EQ(g.getMoveCount(), before);
    EXPECT_NE(g.getBoard().getPieceAt(4, 0), nullptr);
    EXPECT_NE(g.getBoard().getPieceAt(7, 0), nullptr);
    RemoveFile(path);
}

TEST(GameIntegrationTest, EnPassantCaptureAndUndo) {
    Game g;
    g.start();
    g.makeMove(6, 0, 5, 2); // Nf3
    g.makeMove(3, 6, 3, 4); // ...d5
    g.makeMove(5, 2, 6, 0); // Ng1
    g.makeMove(3, 4, 3, 3); // ...d4
    g.makeMove(4, 1, 4, 3); // e4 (double)
    auto targetBefore = g.getEnPassantTarget();
    ASSERT_TRUE(targetBefore.has_value());
    EXPECT_EQ(*targetBefore, std::make_pair(4, 2));

    int before = g.getMoveCount();
    g.makeMove(3, 3, 4, 2); // dxe3 e.p.
    EXPECT_EQ(g.getMoveCount(), before + 1);
    auto blackPawn = g.getBoard().getPieceAt(4, 2);
    ASSERT_NE(blackPawn, nullptr);
    EXPECT_EQ(blackPawn->getColor(), Color::Black);
    EXPECT_EQ(g.getBoard().getPieceAt(4, 3), nullptr);
    EXPECT_FALSE(g.getEnPassantTarget().has_value());

    g.undoMove();
    EXPECT_EQ(g.getMoveCount(), before);
    auto blackPawnRestored = g.getBoard().getPieceAt(3, 3);
    auto whitePawnRestored = g.getBoard().getPieceAt(4, 3);
    ASSERT_NE(blackPawnRestored, nullptr);
    ASSERT_NE(whitePawnRestored, nullptr);
    EXPECT_EQ(blackPawnRestored->getColor(), Color::Black);
    EXPECT_EQ(whitePawnRestored->getColor(), Color::White);
    auto epRestored = g.getEnPassantTarget();
    ASSERT_TRUE(epRestored.has_value());
    EXPECT_EQ(*epRestored, std::make_pair(4, 2));
    EXPECT_FALSE(g.isWhiteTurn()); // back to black's turn after undo
}

TEST(GameIntegrationTest, PinnedPieceMoveIsRejected) {
    nlohmann::json j;
    j["turn"] = "white";
    j["move_count"] = 0;
    j["white_name"] = "White";
    j["black_name"] = "Black";
    j["en_passant"] = nullptr;
    std::vector<std::vector<std::string>> board(8, std::vector<std::string>(8, "."));
    board[0][4] = "K"; // e1
    board[1][4] = "R"; // e2
    board[7][4] = "r"; // e8
    j["board"] = board;
    auto path = WritePositionToTempFile(j);

    Game g;
    g.loadFromFile(path);
    int before = g.getMoveCount();
    g.makeMove(4, 1, 7, 1); // Re2->h2 exposes king
    EXPECT_EQ(g.getMoveCount(), before);
    EXPECT_NE(g.getBoard().getPieceAt(4, 0), nullptr);
    EXPECT_NE(g.getBoard().getPieceAt(4, 1), nullptr);

    RemoveFile(path);
}

TEST(GameIntegrationTest, SimpleCheckmateDetection) {
    nlohmann::json j;
    j["turn"] = "black";
    j["move_count"] = 0;
    j["white_name"] = "White";
    j["black_name"] = "Black";
    j["en_passant"] = nullptr;
    std::vector<std::vector<std::string>> board(8, std::vector<std::string>(8, "."));
    board[7][7] = "k"; // h8
    board[6][5] = "K"; // f7
    board[6][6] = "Q"; // g7
    j["board"] = board;
    auto path = WritePositionToTempFile(j);

    Game g;
    g.loadFromFile(path);
    EXPECT_TRUE(g.isCheckmate());
    EXPECT_FALSE(g.isStalemate());
    RemoveFile(path);
}

TEST(GameIntegrationTest, SimpleStalemateDetection) {
    nlohmann::json j;
    j["turn"] = "black";
    j["move_count"] = 0;
    j["white_name"] = "White";
    j["black_name"] = "Black";
    j["en_passant"] = nullptr;
    std::vector<std::vector<std::string>> board(8, std::vector<std::string>(8, "."));
    board[7][0] = "k"; // a8
    board[6][2] = "K"; // c7
    board[5][1] = "Q"; // b6
    j["board"] = board;
    auto path = WritePositionToTempFile(j);

    Game g;
    g.loadFromFile(path);
    EXPECT_TRUE(g.isStalemate());
    EXPECT_FALSE(g.isCheckmate());
    RemoveFile(path);
}

TEST(GameIntegrationTest, SaveLoadRoundTripPreservesState) {
    Game g;
    g.start();
    // Build en passant-enabled position (after white e4).
    g.makeMove(6, 0, 5, 2); // Nf3
    g.makeMove(3, 6, 3, 4); // ...d5
    g.makeMove(5, 2, 6, 0); // Ng1
    g.makeMove(3, 4, 3, 3); // ...d4
    g.makeMove(4, 1, 4, 3); // e4
    auto epBefore = g.getEnPassantTarget();
    ASSERT_TRUE(epBefore.has_value());

    std::string path = "roundtrip_test_save.json";
    g.saveToFile(path);

    Game g2;
    g2.loadFromFile(path);

    EXPECT_EQ(g2.getMoveCount(), g.getMoveCount());
    EXPECT_EQ(g2.getCurrentPlayer(), g.getCurrentPlayer());
    auto epAfter = g2.getEnPassantTarget();
    ASSERT_TRUE(epAfter.has_value());
    EXPECT_EQ(*epAfter, *epBefore);

    ExpectBoardsEqual(g2.getBoard(), g.getBoard());
    RemoveFile(path);
}

TEST(GameIntegrationTest, PromotionAndUndo) {
    nlohmann::json j;
    j["turn"] = "white";
    j["move_count"] = 0;
    j["white_name"] = "White";
    j["black_name"] = "Black";
    j["en_passant"] = nullptr;
    std::vector<std::vector<std::string>> board(8, std::vector<std::string>(8, "."));
    board[0][6] = "K"; // g1 white king
    board[6][4] = "P"; // e7 white pawn
    board[7][0] = "k"; // a8 black king
    j["board"] = board;
    auto path = WritePositionToTempFile(j);

    Game g;
    g.loadFromFile(path);
    int before = g.getMoveCount();
    g.makeMove(4, 6, 4, 7, PieceType::Knight); // e7 -> e8=N

    EXPECT_EQ(g.getMoveCount(), before + 1);
    auto promoted = g.getBoard().getPieceAt(4, 7);
    ASSERT_NE(promoted, nullptr);
    EXPECT_EQ(promoted->getType(), PieceType::Knight);
    EXPECT_EQ(promoted->getColor(), Color::White);
    EXPECT_TRUE(promoted->hasMoved());

    g.undoMove();
    EXPECT_EQ(g.getMoveCount(), before);
    auto pawn = g.getBoard().getPieceAt(4, 6);
    ASSERT_NE(pawn, nullptr);
    EXPECT_EQ(pawn->getType(), PieceType::Pawn);
    EXPECT_EQ(pawn->getColor(), Color::White);
    EXPECT_FALSE(pawn->hasMoved());
    EXPECT_EQ(g.getBoard().getPieceAt(4, 7), nullptr);
    EXPECT_TRUE(g.isWhiteTurn());
    RemoveFile(path);
}

TEST(GameIntegrationTest, ScholarsMateFromStart) {
    Game g;
    g.start();
    g.makeMove(4, 1, 4, 3); // e4
    g.makeMove(4, 6, 4, 4); // ...e5
    g.makeMove(3, 0, 7, 4); // Qh5
    g.makeMove(1, 7, 2, 5); // ...Nc6
    g.makeMove(5, 0, 2, 3); // Bc4
    g.makeMove(6, 7, 5, 5); // ...Nf6
    g.makeMove(7, 4, 5, 6); // Qxf7#

    EXPECT_TRUE(g.isCheckmate());
    EXPECT_FALSE(g.isStalemate());
    EXPECT_EQ(g.getCurrentPlayer(), Color::Black);
    auto queen = g.getBoard().getPieceAt(5, 6);
    ASSERT_NE(queen, nullptr);
    EXPECT_EQ(queen->getType(), PieceType::Queen);
    EXPECT_EQ(queen->getColor(), Color::White);
}

TEST(GameIntegrationTest, QueensideCastlingForbiddenAfterRookMoved) {
    Game g;
    g.start();
    // Move rook a1 away and back to mark it moved.
    g.makeMove(0, 1, 0, 3); // a2->a4 (clear a2/a3)
    g.makeMove(7, 6, 7, 5); // filler
    g.makeMove(0, 0, 0, 2); // Ra1->a3
    g.makeMove(7, 5, 7, 4); // filler
    g.makeMove(0, 2, 0, 0); // Ra3->a1
    g.makeMove(6, 6, 6, 5); // filler
    // Clear path: move knight b1, bishop c1, queen d1.
    g.makeMove(1, 0, 2, 2); // Nb1->c3
    g.makeMove(1, 7, 0, 5); // filler
    g.makeMove(2, 1, 2, 2); // c2->c3 (now c3 occupied, adjust)
    g.undoMove(); // undo to avoid conflict
    g.makeMove(2, 1, 2, 3); // c2->c4
    g.makeMove(2, 7, 0, 6); // filler
    g.makeMove(2, 0, 0, 2); // Bc1->a3 (clears c1)
    g.makeMove(3, 7, 3, 6); // filler
    g.makeMove(3, 0, 3, 1); // Qd1->d2 (clears d1)

    int before = g.getMoveCount();
    g.makeMove(4, 0, 2, 0); // attempt O-O-O
    EXPECT_EQ(g.getMoveCount(), before); // rejected
    auto king = g.getBoard().getPieceAt(4, 0);
    auto rook = g.getBoard().getPieceAt(0, 0);
    ASSERT_NE(king, nullptr);
    ASSERT_NE(rook, nullptr);
    EXPECT_EQ(king->getType(), PieceType::King);
    EXPECT_EQ(rook->getType(), PieceType::Rook);
}

TEST(GameIntegrationTest, UndoAcrossCastlingAndCapture) {
    Game g;
    g.start();
    g.makeMove(4, 1, 4, 3); // e4
    g.makeMove(4, 6, 4, 4); // ...e5
    g.makeMove(6, 0, 5, 2); // Nf3
    g.makeMove(1, 7, 2, 5); // ...Nc6
    g.makeMove(5, 0, 2, 3); // Bc4
    g.makeMove(6, 7, 5, 5); // ...Nf6
    g.makeMove(4, 0, 6, 0); // O-O
    g.makeMove(5, 5, 4, 3); // ...Nxe4 capture pawn

    int afterCapture = g.getMoveCount();
    g.undoMove(); // undo capture
    EXPECT_EQ(g.getMoveCount(), afterCapture - 1);
    auto knight = g.getBoard().getPieceAt(5, 5);
    auto pawn = g.getBoard().getPieceAt(4, 3);
    ASSERT_NE(knight, nullptr);
    ASSERT_NE(pawn, nullptr);
    EXPECT_EQ(knight->getType(), PieceType::Knight);
    EXPECT_EQ(pawn->getType(), PieceType::Pawn);
    EXPECT_FALSE(g.isWhiteTurn()); // back to black to move

    g.undoMove(); // undo castling
    auto king = g.getBoard().getPieceAt(4, 0);
    auto rook = g.getBoard().getPieceAt(7, 0);
    ASSERT_NE(king, nullptr);
    ASSERT_NE(rook, nullptr);
    EXPECT_EQ(king->getType(), PieceType::King);
    EXPECT_EQ(rook->getType(), PieceType::Rook);
    EXPECT_FALSE(king->hasMoved());
    EXPECT_FALSE(rook->hasMoved());
    EXPECT_TRUE(g.isWhiteTurn()); // back to white to move before castling
}
