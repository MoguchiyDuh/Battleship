#pragma once

#include "Config.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace battleship {

struct Position {
  config::GridCoord x{};
  config::GridCoord y{};

  constexpr Position() noexcept = default;

  constexpr Position(config::GridCoord x_val, config::GridCoord y_val) noexcept
      : x(x_val), y(y_val) {}

  // Parse from "A1" format
  explicit Position(std::string_view coords);

  bool operator==(const Position &other) const noexcept {
    return x == other.x && y == other.y;
  }

  bool operator!=(const Position &other) const noexcept {
    return !(*this == other);
  }

  bool is_valid() const noexcept {
    return x < config::GRID_SIZE && y < config::GRID_SIZE;
  }

  // Safe parsing, returns nullopt on invalid input
  static std::optional<Position> try_parse(std::string_view coords) noexcept;

  // Returns "A1" format
  std::string to_string() const;

  // Hash functor for unordered containers
  struct Hash {
    std::size_t operator()(const Position &pos) const noexcept {
      return (static_cast<std::size_t>(pos.x) << 8) |
             static_cast<std::size_t>(pos.y);
    }
  };

  uint8_t manhattan_distance(const Position &other) const noexcept {
    return static_cast<uint8_t>((x > other.x ? x - other.x : other.x - x) +
                                (y > other.y ? y - other.y : other.y - y));
  }

  bool is_adjacent(const Position &other) const noexcept {
    return manhattan_distance(other) == 1;
  }
};

} // namespace battleship

// Allows Position in std::unordered_set/map
template <> struct std::hash<battleship::Position> {
  std::size_t operator()(const battleship::Position &pos) const noexcept {
    return battleship::Position::Hash{}(pos);
  }
};
