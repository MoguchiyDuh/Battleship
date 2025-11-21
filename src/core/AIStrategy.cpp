#include "AIStrategy.hpp"
#include <algorithm>

namespace battleship::ai {

// Use centralized direction constant
using config::CARDINAL_DIRECTIONS;

// ============================================================================
// Easy AI: Pure random shots
// ============================================================================

RandomStrategy::RandomStrategy()
    : m_rng(std::random_device{}()), m_dist(0, config::GRID_SIZE - 1) {}

Position RandomStrategy::get_attack_position(
    const std::unordered_set<Position, Position::Hash> &attacked_positions,
    [[maybe_unused]] const std::vector<Position> &successful_hits) {

  Position pos;
  constexpr int MAX_ATTEMPTS = 100;
  int attempts = 0;

  do {
    pos = Position{m_dist(m_rng), m_dist(m_rng)};
    ++attempts;
    if (attempts > MAX_ATTEMPTS) {
      throw std::runtime_error("AI failed to find valid attack position");
    }
  } while (attacked_positions.contains(pos));

  return pos;
}

void RandomStrategy::on_attack_result([[maybe_unused]] const Position &pos,
                                      [[maybe_unused]] AttackResult result) {}

// ============================================================================
// Medium AI: Random until hit, then check adjacent, track direction on 2+ hits
// ============================================================================

HuntStrategy::HuntStrategy()
    : m_rng(std::random_device{}()), m_dist(0, config::GRID_SIZE - 1) {}

Position HuntStrategy::get_attack_position(
    const std::unordered_set<Position, Position::Hash> &attacked_positions,
    [[maybe_unused]] const std::vector<Position> &successful_hits) {

  // If we have hunt targets from previous hits, try them first
  while (!m_hunt_targets.empty()) {
    const Position target = m_hunt_targets.back();
    m_hunt_targets.pop_back();

    if (!attacked_positions.contains(target) && target.is_valid()) {
      return target;
    }
  }

  // Fall back to random
  return get_random_position(attacked_positions);
}

void HuntStrategy::on_attack_result(const Position &pos, AttackResult result) {
  if (result == AttackResult::HIT) {
    // Add adjacent cells to hunt targets
    for (const auto &[dx, dy] : CARDINAL_DIRECTIONS) {
      const int new_x = pos.x + dx;
      const int new_y = pos.y + dy;

      if (new_x >= 0 && new_x < config::GRID_SIZE && new_y >= 0 &&
          new_y < config::GRID_SIZE) {
        m_hunt_targets.emplace_back(static_cast<config::GridCoord>(new_x),
                                    static_cast<config::GridCoord>(new_y));
      }
    }
  } else if (result == AttackResult::SUNK) {
    // Ship sunk, clear targets and go back to random
    m_hunt_targets.clear();
  }
}

std::optional<Position> HuntStrategy::find_adjacent_target(
    const Position &hit_pos,
    const std::unordered_set<Position, Position::Hash> &attacked) const {

  for (const auto &[dx, dy] : CARDINAL_DIRECTIONS) {
    const int new_x = hit_pos.x + dx;
    const int new_y = hit_pos.y + dy;

    if (new_x >= 0 && new_x < config::GRID_SIZE && new_y >= 0 &&
        new_y < config::GRID_SIZE) {
      const Position candidate{static_cast<config::GridCoord>(new_x),
                               static_cast<config::GridCoord>(new_y)};
      if (!attacked.contains(candidate)) {
        return candidate;
      }
    }
  }
  return std::nullopt;
}

Position HuntStrategy::get_random_position(
    const std::unordered_set<Position, Position::Hash> &attacked) const {

  Position pos;
  constexpr int MAX_ATTEMPTS = 100;
  int attempts = 0;

  do {
    pos = Position{m_dist(m_rng), m_dist(m_rng)};
    ++attempts;
    if (attempts > MAX_ATTEMPTS) {
      throw std::runtime_error("AI failed to find valid attack position");
    }
  } while (attacked.contains(pos));

  return pos;
}

// ============================================================================
// Hard AI: Chessboard pattern hunt + directional targeting
// ============================================================================

TargetStrategy::TargetStrategy()
    : m_rng(std::random_device{}()), m_dist(0, config::GRID_SIZE - 1) {
  // Pre-build chessboard pattern (50 cells)
  m_chessboard_cells.reserve(50);
  for (config::GridCoord y = 0; y < config::GRID_SIZE; ++y) {
    for (config::GridCoord x = 0; x < config::GRID_SIZE; ++x) {
      if ((x + y) % 2 == 0) {
        m_chessboard_cells.emplace_back(x, y);
      }
    }
  }
  m_chessboard_dirty = false;
}

Position TargetStrategy::get_attack_position(
    const std::unordered_set<Position, Position::Hash> &attacked_positions,
    [[maybe_unused]] const std::vector<Position> &successful_hits) {

  // Target mode: continue destroying current ship
  if (m_mode == Mode::TARGET) {
    if (auto target = get_target_position(attacked_positions); target) {
      return *target;
    }
    // No valid target found, check hunt targets
    while (!m_hunt_targets.empty()) {
      const Position target = m_hunt_targets.back();
      m_hunt_targets.pop_back();
      if (!attacked_positions.contains(target) && target.is_valid()) {
        return target;
      }
    }
    // Fall back to hunt mode
    reset_target_mode();
  }

  // Hunt mode: use chessboard pattern to find ships faster
  return get_random_position(attacked_positions);
}

void TargetStrategy::on_attack_result(const Position &pos,
                                      AttackResult result) {
  if (result == AttackResult::HIT) {
    m_current_ship_hits.emplace_back(pos);
    m_mode = Mode::TARGET;
    update_direction();

    // Add adjacent cells if no direction yet
    if (m_direction == Direction::NONE) {
      for (const auto &[dx, dy] : CARDINAL_DIRECTIONS) {
        const int new_x = pos.x + dx;
        const int new_y = pos.y + dy;

        if (new_x >= 0 && new_x < config::GRID_SIZE && new_y >= 0 &&
            new_y < config::GRID_SIZE) {
          m_hunt_targets.emplace_back(static_cast<config::GridCoord>(new_x),
                                      static_cast<config::GridCoord>(new_y));
        }
      }
    }
  } else if (result == AttackResult::SUNK) {
    reset_target_mode();
  }
}

std::optional<Position> TargetStrategy::get_target_position(
    const std::unordered_set<Position, Position::Hash> &attacked) const {

  if (m_current_ship_hits.empty()) {
    return std::nullopt;
  }

  if (m_direction != Direction::NONE && m_current_ship_hits.size() >= 2) {
    return find_directional_target(attacked);
  }

  return std::nullopt;
}

std::optional<Position> TargetStrategy::find_directional_target(
    const std::unordered_set<Position, Position::Hash> &attacked) const {

  if (m_current_ship_hits.size() < 2) {
    return std::nullopt;
  }

  // Use minmax_element instead of copying and sorting
  if (m_direction == Direction::HORIZONTAL) {
    auto [min_it, max_it] = std::minmax_element(
        m_current_ship_hits.begin(), m_current_ship_hits.end(),
        [](const Position &a, const Position &b) { return a.x < b.x; });

    // Try extending right first, then left
    if (max_it->x + 1 < config::GRID_SIZE) {
      Position right{static_cast<config::GridCoord>(max_it->x + 1), max_it->y};
      if (!attacked.contains(right)) {
        return right;
      }
    }
    if (min_it->x > 0) {
      Position left{static_cast<config::GridCoord>(min_it->x - 1), min_it->y};
      if (!attacked.contains(left)) {
        return left;
      }
    }
  } else if (m_direction == Direction::VERTICAL) {
    auto [min_it, max_it] = std::minmax_element(
        m_current_ship_hits.begin(), m_current_ship_hits.end(),
        [](const Position &a, const Position &b) { return a.y < b.y; });

    // Try extending down first, then up
    if (max_it->y + 1 < config::GRID_SIZE) {
      Position down{max_it->x, static_cast<config::GridCoord>(max_it->y + 1)};
      if (!attacked.contains(down)) {
        return down;
      }
    }
    if (min_it->y > 0) {
      Position up{min_it->x, static_cast<config::GridCoord>(min_it->y - 1)};
      if (!attacked.contains(up)) {
        return up;
      }
    }
  }

  return std::nullopt;
}

Position TargetStrategy::get_random_position(
    const std::unordered_set<Position, Position::Hash> &attacked) const {

  // Use cached chessboard pattern, filter out attacked cells
  std::vector<Position> available;
  available.reserve(m_chessboard_cells.size());

  for (const auto &pos : m_chessboard_cells) {
    if (!attacked.contains(pos)) {
      available.emplace_back(pos);
    }
  }

  if (!available.empty()) {
    std::uniform_int_distribution<std::size_t> idx_dist(0,
                                                        available.size() - 1);
    return available[idx_dist(m_rng)];
  }

  // Fallback: any remaining cell (non-chessboard)
  for (config::GridCoord y = 0; y < config::GRID_SIZE; ++y) {
    for (config::GridCoord x = 0; x < config::GRID_SIZE; ++x) {
      Position pos{x, y};
      if (!attacked.contains(pos)) {
        return pos;
      }
    }
  }

  throw std::runtime_error("AI failed to find valid attack position");
}

void TargetStrategy::update_direction() {
  if (m_current_ship_hits.size() < 2) {
    m_direction = Direction::NONE;
    return;
  }

  const Position &first = m_current_ship_hits[0];
  const Position &second = m_current_ship_hits[1];

  if (first.y == second.y) {
    m_direction = Direction::HORIZONTAL;
  } else if (first.x == second.x) {
    m_direction = Direction::VERTICAL;
  } else {
    // Hits not aligned (shouldn't happen), reset
    m_direction = Direction::NONE;
  }
}

void TargetStrategy::reset_target_mode() {
  m_mode = Mode::HUNT;
  m_direction = Direction::NONE;
  m_current_ship_hits.clear();
  m_hunt_targets.clear();
}

} // namespace battleship::ai
