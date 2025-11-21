#pragma once

#include "Board.hpp"
#include "Config.hpp"
#include "Position.hpp"
#include <memory>
#include <optional>
#include <random>
#include <unordered_set>
#include <vector>

namespace battleship::ai {

// Base strategy interface for AI attacks
class AttackStrategy {
public:
  virtual ~AttackStrategy() = default;

  virtual Position get_attack_position(
      const std::unordered_set<Position, Position::Hash> &attacked_positions,
      const std::vector<Position> &successful_hits) = 0;

  virtual void on_attack_result(const Position &pos, AttackResult result) = 0;
};

// Easy: pure random attacks
class RandomStrategy final : public AttackStrategy {
public:
  RandomStrategy();

  Position get_attack_position(
      const std::unordered_set<Position, Position::Hash> &attacked_positions,
      const std::vector<Position> &successful_hits) override;

  void on_attack_result(const Position &pos, AttackResult result) override;

private:
  mutable std::mt19937 m_rng;
  mutable std::uniform_int_distribution<config::GridCoord> m_dist;
};

// Medium: random until hit, then check adjacent cells
class HuntStrategy final : public AttackStrategy {
public:
  HuntStrategy();

  Position get_attack_position(
      const std::unordered_set<Position, Position::Hash> &attacked_positions,
      const std::vector<Position> &successful_hits) override;

  void on_attack_result(const Position &pos, AttackResult result) override;

private:
  mutable std::mt19937 m_rng;
  mutable std::uniform_int_distribution<config::GridCoord> m_dist;
  std::vector<Position> m_hunt_targets;  // adjacent cells to check

  std::optional<Position> find_adjacent_target(
      const Position &hit_pos,
      const std::unordered_set<Position, Position::Hash> &attacked) const;

  Position get_random_position(
      const std::unordered_set<Position, Position::Hash> &attacked) const;
};

// Hard: tracks ship direction after multiple hits
class TargetStrategy final : public AttackStrategy {
public:
  TargetStrategy();

  Position get_attack_position(
      const std::unordered_set<Position, Position::Hash> &attacked_positions,
      const std::vector<Position> &successful_hits) override;

  void on_attack_result(const Position &pos, AttackResult result) override;

private:
  enum class Mode { HUNT, TARGET };
  enum class Direction { NONE, HORIZONTAL, VERTICAL };

  mutable std::mt19937 m_rng;
  mutable std::uniform_int_distribution<config::GridCoord> m_dist;

  Mode m_mode{Mode::HUNT};
  Direction m_direction{Direction::NONE};

  std::vector<Position> m_current_ship_hits;  // hits on current ship
  std::vector<Position> m_hunt_targets;
  std::vector<Position> m_chessboard_cells;   // cached chessboard pattern
  bool m_chessboard_dirty{true};              // rebuild flag

  std::optional<Position> get_target_position(
      const std::unordered_set<Position, Position::Hash> &attacked) const;

  std::optional<Position> find_directional_target(
      const std::unordered_set<Position, Position::Hash> &attacked) const;

  Position get_random_position(
      const std::unordered_set<Position, Position::Hash> &attacked) const;

  void update_direction();
  void reset_target_mode();
};

// Factory function
inline std::unique_ptr<AttackStrategy> make_strategy(config::Difficulty difficulty) {
  switch (difficulty) {
  case config::Difficulty::EASY:
    return std::make_unique<RandomStrategy>();
  case config::Difficulty::MEDIUM:
    return std::make_unique<HuntStrategy>();
  case config::Difficulty::HARD:
    return std::make_unique<TargetStrategy>();
  default:
    return std::make_unique<RandomStrategy>();
  }
}

} // namespace battleship::ai
