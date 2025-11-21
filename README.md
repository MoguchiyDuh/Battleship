# Battleship

Classic Battleship game in C++20 with AI opponents and online multiplayer.

## Features

- **Game Modes**: Local PvP, PvE (3 difficulties), AI vs AI, Online PvP
- **AI Levels**: Random → Hunt/Target → Chessboard pattern with directional tracking
- **Standard Rules**: 10x10 grid, 10 ships (1×4, 2×3, 3×2, 4×1), no adjacent placement

## Build

```bash
git clone https://github.com/MoguchiyDuh/Battleship.git

cd battleship

cmake -B build && cmake --build build --config Release

./build/battleship
```

Requires: C++20 compiler, Boost.ASIO (for networking)

## Controls

- Attack: `A5`, `J10`, etc.
- Symbols: `~` water, `S` ship, `X` hit, `O` miss, `#` sunk

## Structure

```
include/          # Headers
src/core/         # Game logic (Board, Player, AI, Renderer)
src/net/          # Network layer (Boost.ASIO TCP)
```
