# Battleship

A modern C++20 implementation of the classic Battleship game with AI opponents.

## Features

- **Multiple Game Modes**
  - Player vs Player (PvP)
  - Player vs Computer (Easy/Medium/Hard)
  - AI vs AI (spectate mode)

- **Smart AI Opponents**
  - **Easy**: Random attacks
  - **Medium**: Hunts adjacent cells after hits
  - **Hard**: Chessboard pattern hunting with directional targeting

- **Game Mechanics**
  - Standard 10x10 grid
  - Fleet: 1 Battleship (4), 2 Cruisers (3), 3 Destroyers (2), 4 Patrol Boats (1)
  - Ships cannot touch (including diagonally)
  - Automatic ship placement
  - Real-time battle log and statistics

## Building

Requires C++20 compiler (GCC 10+, Clang 12+, or MSVC 2019+)

```bash
# Clone
git clone https://github.com/MoguchiyDuh/Battleship.git
cd battleship

# Configure with CMake
cmake -B build

# Compile
cmake --build build --config Release

# Run the game
./build/battleship
```

Select your game mode from the menu and play!

## Project Structure

```
├── src/core/
│   ├── Position.cpp      # Grid coordinate handling
│   ├── Ship.cpp          # Ship class with hit detection
│   ├── Board.cpp         # Game board and attack logic
│   ├── Player.cpp        # Player state and actions
│   ├── Game.cpp          # Game loop and flow control
│   └── AIStrategy.cpp    # AI difficulty implementations
├── include/              # Header files
└── CMakeLists.txt
```

## Gameplay

- Enter coordinates like `A5` to attack
- `~` = Water, `S` = Ship, `X` = Hit, `O` = Miss, `#` = Sunk
- Continue attacking on successful hits
- First player to sink all enemy ships wins
