#include "Ship.hpp"
#include <algorithm>

namespace battleship {

Ship::Ship(ShipType type, const Position &start_pos, Orientation orientation)
    : m_type(type), m_orientation(orientation) {
  validate_and_build_positions(start_pos);
}

void Ship::validate_and_build_positions(const Position &start_pos) {
  if (!start_pos.is_valid()) {
    throw std::invalid_argument("Invalid starting position for ship");
  }

  const auto ship_size = size();

  // Check boundaries
  if (m_orientation == Orientation::HORIZONTAL) {
    if (start_pos.x + ship_size > config::GRID_SIZE) {
      throw std::invalid_argument("Ship extends beyond board (horizontal)");
    }
  } else {
    if (start_pos.y + ship_size > config::GRID_SIZE) {
      throw std::invalid_argument("Ship extends beyond board (vertical)");
    }
  }

  // Build positions
  m_position_count = ship_size;
  for (uint8_t i = 0; i < ship_size; ++i) {
    if (m_orientation == Orientation::HORIZONTAL) {
      m_positions[i] = Position{static_cast<config::GridCoord>(start_pos.x + i),
                                start_pos.y};
    } else {
      m_positions[i] = Position{
          start_pos.x, static_cast<config::GridCoord>(start_pos.y + i)};
    }
  }
}

bool Ship::contains(const Position &pos) const noexcept {
  return std::ranges::any_of(positions(), [&pos](const Position &ship_pos) {
    return ship_pos == pos;
  });
}

bool Ship::register_hit(const Position &pos) noexcept {
  if (!contains(pos)) {
    return false;
  }

  if (m_hit_count < size()) {
    ++m_hit_count;
    return true;
  }

  return false;
}

} // namespace battleship
