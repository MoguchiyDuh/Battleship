#pragma once

#include "Player.hpp"
#include <array>
#include <memory>

namespace battleship {

enum class GameMode : uint8_t { PVP, PVE_EASY, PVE_MEDIUM, PVE_HARD, AI_VS_AI };

enum class GameState : uint8_t { SETUP, IN_PROGRESS, GAME_OVER };

// Battle log entry
struct TurnInfo {
  Position attack_pos;
  AttackResult result;
  std::string_view attacker_name;
};

class Game {
public:
  explicit Game(GameMode mode = GameMode::PVE_EASY);

  Game(const Game &) = delete;
  Game &operator=(const Game &) = delete;
  Game(Game &&) = delete;
  Game &operator=(Game &&) = delete;
  ~Game() = default;

  void initialize();
  void start();
  void run_turn();
  bool is_game_over() const noexcept { return m_state == GameState::GAME_OVER; }

  GameState state() const noexcept { return m_state; }
  GameMode mode() const noexcept { return m_mode; }

private:
  GameMode m_mode;
  GameState m_state{GameState::SETUP};

  std::array<std::unique_ptr<Player>, 2> m_players;
  std::size_t m_current_player_index{0};

  static constexpr std::size_t MAX_BATTLE_LOG = 3;
  std::vector<TurnInfo> m_battle_log;

  static constexpr int SHOT_DELAY_MS = 1500;

  void switch_turn() noexcept;
  void handle_shot(const Position &pos);
  void update_game_state();
  void announce_winner() const;
  void sleep_ms(int milliseconds) const;
  void display_game_state() const;

  Player &current_player() noexcept {
    return *m_players[m_current_player_index];
  }
  Player &opponent_player() noexcept {
    return *m_players[1 - m_current_player_index];
  }
  const Player &current_player() const noexcept {
    return *m_players[m_current_player_index];
  }
  const Player &opponent_player() const noexcept {
    return *m_players[1 - m_current_player_index];
  }
};

} // namespace battleship
