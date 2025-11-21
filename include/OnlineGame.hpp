#pragma once

#include "Board.hpp"
#include "Player.hpp"
#include "net/NetworkManager.hpp"
#include <memory>

namespace battleship {

struct TurnInfo;

// Online PvP game - one player per PC
class OnlineGame {
public:
  explicit OnlineGame(net::NetworkManager &network);

  void initialize();
  void run();

private:
  net::NetworkManager &m_network;
  std::unique_ptr<Player> m_local_player;
  Board m_opponent_board; // Tracking opponent's board (hits/misses only)

  std::vector<TurnInfo> m_battle_log;
  static constexpr std::size_t MAX_BATTLE_LOG = 3;
  static constexpr int SHOT_DELAY_MS = 1500;

  bool m_my_turn{false};
  bool m_game_over{false};

  // Track opponent stats
  uint32_t m_opponent_attacks{0};
  uint32_t m_opponent_hits{0};

  // Track opponent remaining ships (starts at full fleet)
  Board::ShipTypeCounts m_opponent_ships{1, 2, 3, 4}; // B:1, C:2, D:3, P:4

  void run_my_turn();
  void run_opponent_turn();
  void display_state() const;
  void sleep_ms(int milliseconds) const;
  void update_opponent_sunk(std::size_t ship_size);
  uint8_t opponent_ships_total() const noexcept;
};

} // namespace battleship
