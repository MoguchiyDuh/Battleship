#pragma once

#include "Config.hpp"
#include "Position.hpp"
#include <array>
#include <memory>
#include <span>

namespace battleship {

enum class Orientation : uint8_t { HORIZONTAL, VERTICAL };

class Ship {
public:
  using ShipType = config::ShipType;

  Ship(ShipType type, const Position &start_pos, Orientation orientation);

  // Non-copyable, movable
  Ship(const Ship &) = delete;
  Ship &operator=(const Ship &) = delete;
  Ship(Ship &&) noexcept = default;
  Ship &operator=(Ship &&) noexcept = default;
  ~Ship() = default;

  bool contains(const Position &pos) const noexcept;
  bool register_hit(const Position &pos) noexcept;  // returns true if hit was new
  bool is_sunk() const noexcept { return m_hit_count >= size(); }

  ShipType type() const noexcept { return m_type; }
  config::GridSize size() const noexcept {
    return static_cast<config::GridSize>(m_type);
  }
  uint8_t hit_count() const noexcept { return m_hit_count; }
  Orientation orientation() const noexcept { return m_orientation; }

  // Zero-copy view of ship's occupied positions
  std::span<const Position> positions() const noexcept {
    return {m_positions.data(), m_position_count};
  }

  auto begin() const noexcept { return m_positions.begin(); }
  auto end() const noexcept { return m_positions.begin() + m_position_count; }

private:
  ShipType m_type;
  Orientation m_orientation;
  uint8_t m_hit_count{0};

  std::array<Position, 4> m_positions{};  // max ship size is 4
  uint8_t m_position_count{0};

  void validate_and_build_positions(const Position &start_pos);
};

inline std::unique_ptr<Ship>
make_ship(config::ShipType type, const Position &pos, Orientation orientation) {
  return std::make_unique<Ship>(type, pos, orientation);
}

} // namespace battleship
