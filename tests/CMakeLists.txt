add_executable(test_game test_game.cpp)
target_link_libraries(test_game gtest_main chess)
include(GoogleTest)
gtest_discover_tests(test_game)
target_include_directories(test_game PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)