#include "Game.hpp"
#include "Renderer.hpp"
#include <chrono>
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

  for (auto &player : m_players) {
    if (player) {
      player->auto_place_ships();
    }
  }

  ConsoleRenderer::display("All ships have been placed automatically.\n");
}

void Game::start() {
  if (!m_players[0] || !m_players[1]) {
    throw std::runtime_error("Game requires exactly 2 players");
  }

  m_state = GameState::IN_PROGRESS;
  ConsoleRenderer::display(Renderer::render_game_start(current_player().name()));
}

void Game::run_turn() {
  if (m_state != GameState::IN_PROGRESS) {
    throw std::runtime_error("Game is not in progress");
  }

  auto &current = current_player();
  auto &opponent = opponent_player();

  current.set_state(PlayerState::ACTIVE);
  opponent.set_state(PlayerState::WAITING);

  bool continue_turn = true;
  while (continue_turn && m_state != GameState::GAME_OVER) {
    ConsoleRenderer::clear();
    display_game_state();

    const Position attack_pos = current.get_attack();
    handle_shot(attack_pos);

    ConsoleRenderer::clear();
    display_game_state();

    update_game_state();

    if (m_state == GameState::GAME_OVER) {
      break;
    }

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

  m_battle_log.emplace_back(TurnInfo{pos, result, current.name()});
}

void Game::update_game_state() {
  if (opponent_player().has_lost()) {
    m_state = GameState::GAME_OVER;
    ConsoleRenderer::clear();
    announce_winner();
  }
}

void Game::announce_winner() const {
  const auto &winner = current_player();
  const auto &loser = opponent_player();

  const std::string output = Renderer::render_game_over(
      winner.name(), loser.name(), winner.board(), loser.board(),
      winner.total_attacks(), winner.accuracy(), loser.total_attacks(),
      loser.accuracy());

  ConsoleRenderer::display(output);
}

void Game::switch_turn() noexcept {
  m_current_player_index = 1 - m_current_player_index;
}

void Game::sleep_ms(int milliseconds) const {
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void Game::display_game_state() const {
  const auto &current = current_player();
  const bool is_pvp = (m_mode == GameMode::PVP);

  std::string output;
  output.reserve(2048);

  output += Renderer::render_header();
  output += Renderer::render_turn(current.name());
  output += Renderer::render_battle_log(m_battle_log, MAX_BATTLE_LOG);

  const std::string left_title = is_pvp ? "PLAYER 1 BOARD" : "YOUR BOARD";
  const std::string right_title = is_pvp ? "PLAYER 2 BOARD" : "COMPUTER'S BOARD";

  output += Renderer::render_boards(m_players[0]->board(), m_players[1]->board(),
                                    left_title, right_title, is_pvp, true);

  const std::string p1_name = is_pvp ? "Player 1" : "Player";
  const std::string p2_name = is_pvp ? "Player 2" : "Computer";
  output += Renderer::render_statistics(m_players[0]->board(),
                                        m_players[1]->board(), p1_name, p2_name);

  ConsoleRenderer::display(output);
}

} // namespace battleship
