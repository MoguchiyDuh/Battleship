#pragma once

#include "Board.hpp"
#include "Player.hpp"
#include "Renderer.hpp"
#include "net/NetworkManager.hpp"
#include <memory>

namespace battleship {

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

  void run_my_turn();
  void run_opponent_turn();
  void display_state() const;
  void sleep_ms(int milliseconds) const;
};

} // namespace battleship
