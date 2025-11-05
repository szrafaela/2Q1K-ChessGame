add_library(chess
    Game.cpp
    Board.cpp
    Piece.cpp
    Player.cpp
    Move.cpp)

target_include_directories(chess PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${CMAKE_SOURCE_DIR}/external/nlohmann_json/single_include
)