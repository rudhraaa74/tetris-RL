cmake_minimum_required(VERSION 3.14)
project(TetrisRL CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- FetchContent for Catch2 ---
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0 # Pinned to a specific version per plan
)
FetchContent_MakeAvailable(Catch2)

# --- Core Engine Library (Headless, purely logic) ---
add_library(tetris_core
    engine/Board.cpp
    engine/Game.cpp
    engine/Piece.cpp
    engine/Randomizer.cpp
    engine/Scoring.cpp
    engine/Timing.cpp
)
target_include_directories(tetris_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# --- Tests ---
add_executable(tetris_test tests/main_test.cpp)
target_link_libraries(tetris_test PRIVATE tetris_core Catch2::Catch2WithMain)
# Add Catch2 test discovery
include(Catch)
catch_discover_tests(tetris_test)

# --- Render/Demo Executable ---
# NOTE: Raylib will be added here later. For now, we stub it out.
# add_executable(tetris_render render/main.cpp engine/Renderer.cpp)
# target_link_libraries(tetris_render PRIVATE tetris_core raylib)
