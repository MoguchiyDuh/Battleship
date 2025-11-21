#include "OnlineGame.hpp"
#include "Game.hpp"
#include "Renderer.hpp"
#include <chrono>
#include <thread>

namespace battleship {

OnlineGame::OnlineGame(net::NetworkManager &network) : m_network(network) {}

void OnlineGame::initialize() {
  const std::string name = m_network.is_host() ? "Host" : "Guest";
  m_local_player = std::make_unique<Player>(name, PlayerType::HUMAN);
  m_local_player->auto_place_ships();

  m_my_turn = m_network.is_host();

  std::string msg = "Ships placed. ";
  msg += m_my_turn ? "You go first!\n" : "Opponent goes first.\n";
  ConsoleRenderer::display(msg);
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

    ConsoleRenderer::display("Your turn! ");
    const Position attack_pos = m_local_player->get_attack();

    if (!m_network.send_attack(attack_pos)) {
      ConsoleRenderer::display("Failed to send attack\n");
      return;
    }

    auto msg = m_network.receive();
    if (!msg) {
      ConsoleRenderer::display("Failed to receive result\n");
      return;
    }

    // Check for game over (we won)
    if (msg->type == net::MessageType::GAME_OVER) {
      m_game_over = true;
      ConsoleRenderer::clear();
      ConsoleRenderer::display(msg->payload);
      return;
    }

    // Handle SUNK with ship positions
    if (msg->type == net::MessageType::RESULT_SUNK) {
      std::vector<Position> ship_cells;
      std::string payload = msg->payload;
      std::size_t pos_start = 0;

      while (pos_start < payload.size()) {
        std::size_t comma = payload.find(',', pos_start);
        std::string pos_str =
            (comma == std::string::npos)
                ? payload.substr(pos_start)
                : payload.substr(pos_start, comma - pos_start);
        if (auto p = Position::try_parse(pos_str)) {
          ship_cells.push_back(*p);
        }
        if (comma == std::string::npos)
          break;
        pos_start = comma + 1;
      }

      m_local_player->record_attack_result(attack_pos, AttackResult::SUNK);
      m_opponent_board.mark_sunk_ship(ship_cells);
      update_opponent_sunk(ship_cells.size());

      m_battle_log.emplace_back(
          TurnInfo{attack_pos, AttackResult::SUNK, m_local_player->name()});
      display_state();

      continue_turn = true;
      sleep_ms(SHOT_DELAY_MS);
      continue;
    }

    if (msg->type != net::MessageType::RESULT || msg->payload.empty()) {
      ConsoleRenderer::display("Unexpected message type\n");
      return;
    }

    const auto result = static_cast<AttackResult>(msg->payload[0]);

    m_local_player->record_attack_result(attack_pos, result);
    m_opponent_board.mark_attack(attack_pos, result);

    m_battle_log.emplace_back(
        TurnInfo{attack_pos, result, m_local_player->name()});

    display_state();

    continue_turn =
        (result == AttackResult::HIT || result == AttackResult::SUNK);

    if (continue_turn) {
      sleep_ms(SHOT_DELAY_MS);
    }
  }

  if (!m_game_over) {
    m_my_turn = false;
    m_network.send_your_turn();
    sleep_ms(SHOT_DELAY_MS);
  }
}

void OnlineGame::run_opponent_turn() {
  ConsoleRenderer::display("Waiting for opponent's attack...\n");

  bool continue_turn = true;
  while (continue_turn && !m_game_over && m_network.is_connected()) {
    auto msg = m_network.receive();
    if (!msg) {
      ConsoleRenderer::display("Connection lost\n");
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

    const Ship *ship = m_local_player->board().get_ship_at(*attack_pos);
    const AttackResult result = m_local_player->receive_attack(*attack_pos);

    ++m_opponent_attacks;
    if (result == AttackResult::HIT || result == AttackResult::SUNK) {
      ++m_opponent_hits;
    }

    m_battle_log.emplace_back(TurnInfo{*attack_pos, result, "Opponent"});
    display_state();

    // Check if we lost
    if (m_local_player->has_lost()) {
      m_game_over = true;

      const float opponent_accuracy =
          m_opponent_attacks > 0
              ? static_cast<float>(m_opponent_hits) / m_opponent_attacks
              : 0.0f;

      const std::string winner_screen = Renderer::render_game_over(
          "You", "Opponent", m_opponent_board, m_local_player->board(),
          m_opponent_attacks, opponent_accuracy, m_local_player->total_attacks(),
          m_local_player->accuracy());

      m_network.send_game_over(winner_screen);

      ConsoleRenderer::clear();
      const std::string loser_screen = Renderer::render_game_over(
          "Opponent", m_local_player->name(), m_opponent_board,
          m_local_player->board(), m_opponent_attacks, opponent_accuracy,
          m_local_player->total_attacks(), m_local_player->accuracy());
      ConsoleRenderer::display(loser_screen);
      return;
    }

    // Send result back
    if (result == AttackResult::SUNK && ship) {
      std::string positions;
      for (const auto &pos : ship->positions()) {
        if (!positions.empty())
          positions += ",";
        positions += pos.to_string();
      }
      net::Message sunk_msg;
      sunk_msg.type = net::MessageType::RESULT_SUNK;
      sunk_msg.payload = positions;
      m_network.send(sunk_msg);
    } else {
      m_network.send_result(static_cast<uint8_t>(result));
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

  std::string output;
  output.reserve(2048);

  output += Renderer::render_header();
  output += m_my_turn ? "\u3010 Your Turn \u3011\n\n" : "\u3010 Opponent's Turn \u3011\n\n";
  output += Renderer::render_battle_log(m_battle_log, MAX_BATTLE_LOG);
  output += Renderer::render_boards(m_local_player->board(), m_opponent_board,
                                    "YOUR BOARD", "OPPONENT'S BOARD", false, true);

  const auto your_counts = m_local_player->board().get_remaining_ship_types();
  const auto your_total = m_local_player->board().ships_remaining();

  output += Renderer::render_statistics(your_counts, your_total, m_opponent_ships,
                                        opponent_ships_total(), "You", "Opponent");

  ConsoleRenderer::display(output);
}

void OnlineGame::sleep_ms(int milliseconds) const {
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void OnlineGame::update_opponent_sunk(std::size_t ship_size) {
  switch (ship_size) {
  case 4:
    if (m_opponent_ships.battleships > 0)
      --m_opponent_ships.battleships;
    break;
  case 3:
    if (m_opponent_ships.cruisers > 0)
      --m_opponent_ships.cruisers;
    break;
  case 2:
    if (m_opponent_ships.destroyers > 0)
      --m_opponent_ships.destroyers;
    break;
  case 1:
    if (m_opponent_ships.patrol_boats > 0)
      --m_opponent_ships.patrol_boats;
    break;
  default:
    break;
  }
}

uint8_t OnlineGame::opponent_ships_total() const noexcept {
  return m_opponent_ships.battleships + m_opponent_ships.cruisers +
         m_opponent_ships.destroyers + m_opponent_ships.patrol_boats;
}

} // namespace battleship
