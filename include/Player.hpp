#pragma once

#include "AIStrategy.hpp"
#include "Board.hpp"
#include "Config.hpp"
#include "Position.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace battleship {

enum class PlayerType : uint8_t { HUMAN, AI };

enum class PlayerState : uint8_t {
  SETUP,
  READY,
  ACTIVE,
  WAITING,
  VICTORY,
  DEFEAT
};

class Player {
public:
  Player(std::string_view name, PlayerType type,
         config::Difficulty ai_difficulty = config::Difficulty::EASY);

  Player(const Player &) = delete;
  Player &operator=(const Player &) = delete;
  Player(Player &&) noexcept = default;
  Player &operator=(Player &&) noexcept = default;
  ~Player() = default;

  bool place_ship(config::ShipType type, const Position &pos,
                  Orientation orientation);
  void auto_place_ships();
  void manual_place_ships();
  bool all_ships_placed() const noexcept;

  Position get_attack();
  AttackResult receive_attack(const Position &pos);
  void record_attack_result(const Position &pos, AttackResult result);

  bool is_ready() const noexcept { return m_state == PlayerState::READY; }
  bool has_lost() const noexcept { return m_board.is_game_over(); }

  std::string_view name() const noexcept { return m_name; }
  PlayerType type() const noexcept { return m_type; }
  PlayerState state() const noexcept { return m_state; }
  const Board &board() const noexcept { return m_board; }
  Board &board() noexcept { return m_board; }

  void set_state(PlayerState new_state) noexcept { m_state = new_state; }

  uint16_t total_attacks() const noexcept { return m_total_attacks; }
  uint16_t successful_hits() const noexcept { return m_successful_hits_count; }
  float accuracy() const noexcept;

private:
  std::string m_name;
  PlayerType m_type;
  PlayerState m_state{PlayerState::SETUP};
  Board m_board;

  std::unique_ptr<ai::AttackStrategy> m_ai_strategy;

  std::unordered_set<Position, Position::Hash> m_attacked_positions;
  std::vector<Position> m_successful_hit_positions;
  uint16_t m_total_attacks{0};
  uint16_t m_successful_hits_count{0};

  Position get_human_attack();
  Position get_ai_attack();

  bool is_valid_attack(const Position &pos) const noexcept;
  bool get_placement_from_user(config::ShipType type, Position &pos,
                               Orientation &orientation);
};

} // namespace battleship
