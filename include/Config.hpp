#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace battleship::config {

using GridCoord = uint8_t;
using GridSize = uint8_t;

inline constexpr GridSize GRID_SIZE = 10;

// Ship type enum where value = ship size
enum class ShipType : uint8_t {
  BATTLESHIP = 4,
  CRUISER = 3,
  DESTROYER = 2,
  PATROL_BOAT = 1
};

struct ShipConfig {
  ShipType type;
  uint8_t count;  // how many of this ship type
  std::string_view name;

  constexpr GridSize size() const noexcept {
    return static_cast<GridSize>(type);
  }
};

// Fleet composition: 1x4, 2x3, 3x2, 4x1
inline constexpr std::array<ShipConfig, 4> SHIP_CONFIGS = {
    {{ShipType::BATTLESHIP, 1, "Battleship"},
     {ShipType::CRUISER, 2, "Cruiser"},
     {ShipType::DESTROYER, 3, "Destroyer"},
     {ShipType::PATROL_BOAT, 4, "Patrol Boat"}}};

inline constexpr uint8_t TOTAL_SHIPS = 10;       // 1+2+3+4
inline constexpr uint8_t TOTAL_SHIP_CELLS = 20;  // 4+6+6+4

enum class Difficulty : uint8_t { EASY = 0, MEDIUM = 1, HARD = 2 };

} // namespace battleship::config
