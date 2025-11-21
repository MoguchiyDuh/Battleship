#include "Game.hpp"
#include "StringUtils.hpp"
#include <chrono>
#include <format>
#include <iostream>
#include <thread>

namespace battleship {

Game::Game(GameMode mode) : m_mode(mode) {}

void Game::initialize() {
  switch (m_mode) {
  case GameMode::PVP:
    m_players[0] = std::make_unique<Player>("Player 1", PlayerType::HUMAN);
    m_players[1] = std::make_unique<Player>("Player 2", PlayerType::HUMAN);
    break;

  case GameMode::PVE_EASY:
    m_players[0] = std::make_unique<Player>("Player", PlayerType::HUMAN);
    m_players[1] = std::make_unique<Player>("Computer", PlayerType::AI,
                                            config::Difficulty::EASY);
    break;

  case GameMode::PVE_MEDIUM:
    m_players[0] = std::make_unique<Player>("Player", PlayerType::HUMAN);
    m_players[1] = std::make_unique<Player>("Computer", PlayerType::AI,
                                            config::Difficulty::MEDIUM);
    break;

  case GameMode::PVE_HARD:
    m_players[0] = std::make_unique<Player>("Player", PlayerType::HUMAN);
    m_players[1] = std::make_unique<Player>("Computer", PlayerType::AI,
                                            config::Difficulty::HARD);
    break;

  case GameMode::AI_VS_AI:
    m_players[0] = std::make_unique<Player>("Computer 1", PlayerType::AI,
                                            config::Difficulty::MEDIUM);
    m_players[1] = std::make_unique<Player>("Computer 2", PlayerType::AI,
                                            config::Difficulty::HARD);
    break;
  }

  // Auto-place ships for all players
  for (auto &player : m_players) {
    if (player) {
      player->auto_place_ships();
    }
  }

  std::cout << "All ships have been placed automatically.\n";
}

void Game::start() {
  if (!m_players[0] || !m_players[1]) {
    throw std::runtime_error("Game requires exactly 2 players");
  }

  m_state = GameState::IN_PROGRESS;
  std::cout << "\n=== BATTLESHIP GAME STARTED ===\n";
  std::cout << std::format("{} goes first!\n", current_player().name());
}

void Game::run_turn() {
  if (m_state != GameState::IN_PROGRESS) {
    throw std::runtime_error("Game is not in progress");
  }

  auto &current = current_player();
  auto &opponent = opponent_player();

  current.set_state(PlayerState::ACTIVE);
  opponent.set_state(PlayerState::WAITING);

  // Keep shooting while hitting/sinking ships
  bool continue_turn = true;
  while (continue_turn && m_state != GameState::GAME_OVER) {
    clear_console();
    display_game_state();

    const Position attack_pos = current.get_attack();
    handle_shot(attack_pos);

    // Update display immediately after shot
    clear_console();
    display_game_state();

    update_game_state();

    if (m_state == GameState::GAME_OVER) {
      break;
    }

    // Continue turn only on HIT or SUNK
    const auto &last_result = m_battle_log.back().result;
    continue_turn =
        (last_result == AttackResult::HIT || last_result == AttackResult::SUNK);

    if (continue_turn) {
      sleep_ms(SHOT_DELAY_MS);
    }
  }

  if (m_state != GameState::GAME_OVER) {
    sleep_ms(SHOT_DELAY_MS);
    switch_turn();
  }
}

void Game::handle_shot(const Position &pos) {
  auto &current = current_player();
  auto &opponent = opponent_player();

  const AttackResult result = opponent.receive_attack(pos);
  current.record_attack_result(pos, result);

  add_to_battle_log(TurnInfo{pos, result, current.name()});
}

void Game::update_game_state() {
  if (opponent_player().has_lost()) {
    m_state = GameState::GAME_OVER;
    clear_console();
    announce_winner();
  }
}

void Game::announce_winner() const {
  const auto &winner = current_player();
  const auto &loser = opponent_player();

  constexpr std::size_t BOX_WIDTH = 51;
  std::cout << "╔" << str::repeat("═", BOX_WIDTH) << "╗\n";
  std::cout << "║" << str::center("BATTLESHIP", BOX_WIDTH) << "║\n";
  std::cout << "╚" << str::repeat("═", BOX_WIDTH) << "╝\n\n";

  std::cout << "【 GAME OVER 】\n\n";
  std::cout << std::format("  {} WINS!\n\n", winner.name());

  std::cout << "【 FINAL BOARDS 】\n";
  std::cout << str::center(std::string(winner.name()), 23) << "       "
            << str::center(std::string(loser.name()), 23) << "\n";

  const auto winner_grid = winner.board().render(false);
  const auto loser_grid = loser.board().render(false);

  std::cout << "   A B C D E F G H I J       " << "   A B C D E F G H I J\n";
  for (uint8_t y = 0; y < Board::GRID_SIZE; ++y) {
    std::cout << std::format("{:2} ", y + 1);
    for (uint8_t x = 0; x < Board::GRID_SIZE; ++x) {
      std::cout << winner_grid[y][x] << ' ';
    }
    std::cout << "       ";
    std::cout << std::format("{:2} ", y + 1);
    for (uint8_t x = 0; x < Board::GRID_SIZE; ++x) {
      std::cout << loser_grid[y][x] << ' ';
    }
    std::cout << '\n';
  }

  std::cout << "\n【 STATISTICS 】\n";
  std::cout << std::format("  {}: {} attacks, {:.1f}% accuracy\n",
                           winner.name(), winner.total_attacks(),
                           winner.accuracy() * 100.0f);
  std::cout << std::format("  {}: {} attacks, {:.1f}% accuracy\n", loser.name(),
                           loser.total_attacks(), loser.accuracy() * 100.0f);
  std::cout << "\n";
}

void Game::switch_turn() noexcept {
  m_current_player_index = 1 - m_current_player_index;
}

void Game::clear_console() const { std::cout << "\033[2J\033[1;1H"; }

void Game::sleep_ms(int milliseconds) const {
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void Game::display_game_state() const {
  const auto &current = current_player();

  constexpr std::size_t BOX_WIDTH = 51;
  std::cout << "╔" << str::repeat("═", BOX_WIDTH) << "╗\n";
  std::cout << "║" << str::center("BATTLESHIP", BOX_WIDTH) << "║\n";
  std::cout << "╚" << str::repeat("═", BOX_WIDTH) << "╝\n\n";

  std::cout << "【 " << current.name() << "'s Turn 】\n\n";

  // Battle log - last 3 shots
  if (!m_battle_log.empty()) {
    std::cout << "【 BATTLE LOG 】\n";
    const std::size_t start_idx = m_battle_log.size() > MAX_BATTLE_LOG
                                      ? m_battle_log.size() - MAX_BATTLE_LOG
                                      : 0;

    for (std::size_t i = start_idx; i < m_battle_log.size(); ++i) {
      const auto &log = m_battle_log[i];
      std::cout << std::format("  {} attacked {} → ", log.attacker_name,
                               log.attack_pos.to_string());

      switch (log.result) {
      case AttackResult::MISS:
        std::cout << "MISS";
        break;
      case AttackResult::HIT:
        std::cout << "HIT!";
        break;
      case AttackResult::SUNK:
        std::cout << "SUNK!";
        break;
      case AttackResult::ALREADY_ATTACKED:
        std::cout << "Already attacked";
        break;
      case AttackResult::INVALID_COORD:
        std::cout << "Invalid";
        break;
      }
      std::cout << "\n";
    }
    std::cout << "\n";
  }

  // Check if PvP mode
  const bool is_pvp = (m_mode == GameMode::PVP);

  // Board width: "   A B C D E F G H I J" = 23 chars
  constexpr std::size_t BOARD_WIDTH = 23;
  constexpr std::size_t GAP_WIDTH = 7;

  const std::string left_title = is_pvp ? "PLAYER 1 BOARD" : "YOUR BOARD";
  const std::string right_title =
      is_pvp ? "PLAYER 2 BOARD" : "COMPUTER'S BOARD";

  std::cout << str::center(left_title, BOARD_WIDTH)
            << std::string(GAP_WIDTH, ' ')
            << str::center(right_title, BOARD_WIDTH) << "\n";

  // Always hide ships (true = hide), show only water/miss/hit/sunk
  const auto player_grid = m_players[0]->board().render(is_pvp ? true : false);
  const auto ai_grid = m_players[1]->board().render(true);

  std::cout << "   A B C D E F G H I J       " << "    A B C D E F G H I J\n";
  for (uint8_t y = 0; y < Board::GRID_SIZE; ++y) {
    // Player board (left)
    std::cout << std::format("{:2} ", y + 1);
    for (uint8_t x = 0; x < Board::GRID_SIZE; ++x) {
      std::cout << player_grid[y][x] << ' ';
    }

    std::cout << "       ";

    // AI board (right)
    std::cout << std::format("{:2} ", y + 1);
    for (uint8_t x = 0; x < Board::GRID_SIZE; ++x) {
      std::cout << ai_grid[y][x] << ' ';
    }
    std::cout << '\n';
  }

  std::cout << std::format("\n【 STATISTICS 】\n");

  const auto p1_counts = m_players[0]->board().get_remaining_ship_types();
  const auto p2_counts = m_players[1]->board().get_remaining_ship_types();
  const auto p1_total = m_players[0]->board().ships_remaining();
  const auto p2_total = m_players[1]->board().ships_remaining();

  if (is_pvp) {
    std::cout << std::format("  Player 1: {} ships (B:{} C:{} D:{} P:{})\n",
                             p1_total, p1_counts.battleships,
                             p1_counts.cruisers, p1_counts.destroyers,
                             p1_counts.patrol_boats);
    std::cout << std::format("  Player 2: {} ships (B:{} C:{} D:{} P:{})\n",
                             p2_total, p2_counts.battleships,
                             p2_counts.cruisers, p2_counts.destroyers,
                             p2_counts.patrol_boats);
  } else {
    std::cout << std::format("  Player: {} ships (B:{} C:{} D:{} P:{})\n",
                             p1_total, p1_counts.battleships,
                             p1_counts.cruisers, p1_counts.destroyers,
                             p1_counts.patrol_boats);
    std::cout << std::format("  Computer: {} ships (B:{} C:{} D:{} P:{})\n",
                             p2_total, p2_counts.battleships,
                             p2_counts.cruisers, p2_counts.destroyers,
                             p2_counts.patrol_boats);
  }
  std::cout << "\n";
}

void Game::add_to_battle_log(const TurnInfo &info) {
  m_battle_log.emplace_back(info);
}

} // namespace battleship
