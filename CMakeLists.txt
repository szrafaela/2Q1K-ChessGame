cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0135 NEW)
cmake_policy(SET CMP0079 NEW)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(ChessGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF) # Build everything statically so the final exe has no DLL dependencies.

if(MSVC)
  # Link the static runtime (/MT or /MTd) to avoid requiring the Visual C++ redistributable DLLs.
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  set(gtest_force_shared_crt OFF CACHE BOOL "Use shared (DLL) CRT for GoogleTest" FORCE)
elseif(MINGW)
  # Statically link libstdc++, libgcc, and pthreads on MinGW to avoid extra DLLs.
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
endif()

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
