add_library(chess
    Game.cpp
    Board.cpp
    Piece.cpp
    Player.cpp
    Move.cpp)

target_include_directories(chess PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(chess
    PUBLIC
        nlohmann_json::nlohmann_json
)
