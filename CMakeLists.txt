cmake_minimum_required(VERSION 3.20)
project(Battleship VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
    src/main.cpp
    src/core/Position.cpp
    src/core/Ship.cpp
    src/core/Board.cpp
    src/core/Player.cpp
    src/core/Game.cpp
    src/core/AIStrategy.cpp
)

add_executable(battleship ${SOURCES})

target_compile_options(battleship PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -O3
    -flto
)

if(NOT ANDROID)
    target_compile_options(battleship PRIVATE -march=native)
endif()

target_link_options(battleship PRIVATE
    -flto
)
