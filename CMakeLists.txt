cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0135 NEW)
cmake_policy(SET CMP0079 NEW)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(ChessGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# GoogleTest letöltése és előkészítése
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/release-1.12.1.zip
)
FetchContent_MakeAvailable(googletest)

# nlohmann/json letöltése és elérhetővé tétele
FetchContent_Declare(
  json
  URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.2.zip
)
FetchContent_MakeAvailable(json)

# --- Könyvtár a játéklogikához ---
add_subdirectory(src)

# --- Tesztek (Google Test) ---
add_subdirectory(tests)

# --- Futtatható alkalmazás ---
add_executable(chess_app src/main.cpp)

# --- Linkelés a könyvtárhoz és JSON-hoz ---
target_link_libraries(chess_app PRIVATE chess nlohmann_json::nlohmann_json)
