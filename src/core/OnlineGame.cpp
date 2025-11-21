#include "OnlineGame.hpp"
#include "Game.hpp"
#include <chrono>
#include <iostream>
#include <thread>

namespace battleship {

OnlineGame::OnlineGame(net::NetworkManager &network) : m_network(network) {}

void OnlineGame::initialize() {
  const std::string name = m_network.is_host() ? "Host" : "Guest";
  m_local_player = std::make_unique<Player>(name, PlayerType::HUMAN);
  m_local_player->auto_place_ships();

  // Host goes first
  m_my_turn = m_network.is_host();

  std::cout << "Ships placed. ";
  if (m_my_turn) {
    std::cout << "You go first!\n";
  } else {
    std::cout << "Opponent goes first.\n";
  }
}

void OnlineGame::run() {
  while (!m_game_over && m_network.is_connected()) {
    display_state();

    if (m_my_turn) {
      run_my_turn();
    } else {
      run_opponent_turn();
    }
  }
}

void OnlineGame::run_my_turn() {
  m_local_player->set_state(PlayerState::ACTIVE);

  bool continue_turn = true;
  while (continue_turn && !m_game_over && m_network.is_connected()) {
    display_state();

    // Get attack from local player
    std::cout << "Your turn! ";
    const Position attack_pos = m_local_player->get_attack();

    // Send attack to opponent
    if (!m_network.send_attack(attack_pos)) {
      std::cerr << "Failed to send attack\n";
      return;
    }

    // Wait for result
    auto result_opt = m_network.receive_result();
    if (!result_opt) {
      std::cerr << "Failed to receive result\n";
      return;
    }

    const auto result = static_cast<AttackResult>(*result_opt);

    // Update local tracking
    m_local_player->record_attack_result(attack_pos, result);
    m_opponent_board.mark_attack(attack_pos, result);

    m_battle_log.emplace_back(
        TurnInfo{attack_pos, result, m_local_player->name()});

    display_state();

    // Check if opponent lost
    if (result == AttackResult::SUNK) {
      auto msg = m_network.receive();
      if (msg && msg->type == net::MessageType::GAME_OVER) {
        m_game_over = true;
        ConsoleRenderer::clear();
        ConsoleRenderer::display(msg->payload);
        return;
      }
    }

    // Continue on hit/sunk
    continue_turn =
        (result == AttackResult::HIT || result == AttackResult::SUNK);

    if (continue_turn) {
      sleep_ms(SHOT_DELAY_MS);
    }
  }

  // Switch turn
  if (!m_game_over) {
    m_my_turn = false;
    m_network.send_your_turn();
    sleep_ms(SHOT_DELAY_MS);
  }
}

void OnlineGame::run_opponent_turn() {
  std::cout << "Waiting for opponent's attack...\n";

  bool continue_turn = true;
  while (continue_turn && !m_game_over && m_network.is_connected()) {
    // Receive attack
    auto msg = m_network.receive();
    if (!msg) {
      std::cerr << "Connection lost\n";
      return;
    }

    if (msg->type == net::MessageType::YOUR_TURN) {
      m_my_turn = true;
      return;
    }

    if (msg->type != net::MessageType::ATTACK) {
      continue;
    }

    auto attack_pos = Position::try_parse(msg->payload);
    if (!attack_pos) {
      continue;
    }

    // Process attack on our board
    const AttackResult result = m_local_player->receive_attack(*attack_pos);

    // Send result back
    m_network.send_result(static_cast<uint8_t>(result));

    m_battle_log.emplace_back(TurnInfo{*attack_pos, result, "Opponent"});

    display_state();

    // Check if we lost
    if (m_local_player->has_lost()) {
      m_game_over = true;

      // Send game over screen
      const std::string game_over = Renderer::render_game_over(
          "Opponent", m_local_player->name(), m_opponent_board,
          m_local_player->board(), 0, 0.0f, m_local_player->total_attacks(),
          m_local_player->accuracy());

      m_network.send_game_over(game_over);

      ConsoleRenderer::clear();
      std::cout << "【 GAME OVER 】\n\nYou lost!\n";
      return;
    }

    continue_turn =
        (result == AttackResult::HIT || result == AttackResult::SUNK);

    if (continue_turn) {
      sleep_ms(SHOT_DELAY_MS);
    }
  }
}

void OnlineGame::display_state() const {
  ConsoleRenderer::clear();

  std::string output = Renderer::render_header();

  if (m_my_turn) {
    output += "【 Your Turn 】\n\n";
  } else {
    output += "【 Opponent's Turn 】\n\n";
  }

  output += Renderer::render_battle_log(m_battle_log, MAX_BATTLE_LOG);

  output +=
      Renderer::render_boards(m_local_player->board(), m_opponent_board,
                              "YOUR BOARD", "OPPONENT'S BOARD", false, true);

  output += Renderer::render_statistics(m_local_player->board(),
                                        m_opponent_board, "You", "Opponent");

  ConsoleRenderer::display(output);
}

void OnlineGame::sleep_ms(int milliseconds) const {
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

} // namespace battleship
