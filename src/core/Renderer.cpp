#include "Renderer.hpp"
#include "Game.hpp"
#include "StringUtils.hpp"
#include <format>
#include <iostream>

namespace battleship {

std::string Renderer::render_header() {
  std::string result;
  result.reserve(128);
  result += "\u2554" + str::repeat("\u2550", BOX_WIDTH) + "\u2557\n";
  result += "\u2551" + str::center("BATTLESHIP", BOX_WIDTH) + "\u2551\n";
  result += "\u255A" + str::repeat("\u2550", BOX_WIDTH) + "\u255D\n\n";
  return result;
}

std::string Renderer::render_turn(std::string_view player_name) {
  return std::format("\u3010 {}'s Turn \u3011\n\n", player_name);
}

std::string Renderer::result_to_string(AttackResult result) {
  switch (result) {
  case AttackResult::MISS:
    return "MISS";
  case AttackResult::HIT:
    return "HIT!";
  case AttackResult::SUNK:
    return "SUNK!";
  case AttackResult::ALREADY_ATTACKED:
    return "Already attacked";
  case AttackResult::INVALID_COORD:
    return "Invalid";
  }
  return "Unknown";
}

std::string Renderer::render_battle_log(const std::vector<TurnInfo> &log,
                                        std::size_t max_entries) {
  if (log.empty()) {
    return "";
  }

  std::string result = "\u3010 BATTLE LOG \u3011\n";
  const std::size_t start_idx =
      log.size() > max_entries ? log.size() - max_entries : 0;

  for (std::size_t i = start_idx; i < log.size(); ++i) {
    const auto &entry = log[i];
    result += std::format("  {} attacked {} \u2192 {}\n", entry.attacker_name,
                          entry.attack_pos.to_string(),
                          result_to_string(entry.result));
  }
  result += "\n";
  return result;
}

std::string Renderer::render_single_board(const Board::DisplayGrid &grid) {
  std::string result;
  result.reserve(256);
  result += "   A B C D E F G H I J\n";
  for (uint8_t y = 0; y < Board::GRID_SIZE; ++y) {
    result += std::format("{:2} ", y + 1);
    for (uint8_t x = 0; x < Board::GRID_SIZE; ++x) {
      result += grid[y][x];
      result += ' ';
    }
    result += '\n';
  }
  return result;
}

std::string Renderer::render_boards(const Board &left_board,
                                    const Board &right_board,
                                    std::string_view left_title,
                                    std::string_view right_title,
                                    bool hide_left_ships,
                                    bool hide_right_ships) {
  std::string result;
  result.reserve(512);

  // Titles
  result += str::center(std::string(left_title), BOARD_WIDTH);
  result += std::string(GAP_WIDTH, ' ');
  result += str::center(std::string(right_title), BOARD_WIDTH) + "\n";

  // Get rendered grids
  const auto left_grid = left_board.render(hide_left_ships);
  const auto right_grid = right_board.render(hide_right_ships);

  // Column headers
  result += "   A B C D E F G H I J       ";
  result += "   A B C D E F G H I J\n";

  // Rows side by side
  for (uint8_t y = 0; y < Board::GRID_SIZE; ++y) {
    result += std::format("{:2} ", y + 1);
    for (uint8_t x = 0; x < Board::GRID_SIZE; ++x) {
      result += left_grid[y][x];
      result += ' ';
    }
    result += "       ";
    result += std::format("{:2} ", y + 1);
    for (uint8_t x = 0; x < Board::GRID_SIZE; ++x) {
      result += right_grid[y][x];
      result += ' ';
    }
    result += '\n';
  }

  return result;
}

std::string Renderer::render_statistics(const Board &player_board,
                                        const Board &opponent_board,
                                        std::string_view player_name,
                                        std::string_view opponent_name) {
  const auto p_counts = player_board.get_remaining_ship_types();
  const auto o_counts = opponent_board.get_remaining_ship_types();
  return render_statistics(p_counts, player_board.ships_remaining(), o_counts,
                           opponent_board.ships_remaining(), player_name,
                           opponent_name);
}

std::string Renderer::render_statistics(const Board::ShipTypeCounts &player_counts,
                                        uint8_t player_total,
                                        const Board::ShipTypeCounts &opponent_counts,
                                        uint8_t opponent_total,
                                        std::string_view player_name,
                                        std::string_view opponent_name) {
  std::string result = "\n\u3010 STATISTICS \u3011\n";
  result += std::format("  {}: {} ships (B:{} C:{} D:{} P:{})\n", player_name,
                        player_total, player_counts.battleships,
                        player_counts.cruisers, player_counts.destroyers,
                        player_counts.patrol_boats);
  result += std::format("  {}: {} ships (B:{} C:{} D:{} P:{})\n", opponent_name,
                        opponent_total, opponent_counts.battleships,
                        opponent_counts.cruisers, opponent_counts.destroyers,
                        opponent_counts.patrol_boats);
  result += "\n";
  return result;
}

std::string Renderer::render_game_over(std::string_view winner_name,
                                       std::string_view loser_name,
                                       const Board &winner_board,
                                       const Board &loser_board,
                                       uint32_t winner_attacks,
                                       float winner_accuracy,
                                       uint32_t loser_attacks,
                                       float loser_accuracy) {
  std::string result = render_header();

  result += "\u3010 GAME OVER \u3011\n\n";
  result += std::format("  {} WINS!\n\n", winner_name);

  result += "\u3010 FINAL BOARDS \u3011\n";
  result += render_boards(winner_board, loser_board, winner_name, loser_name,
                          false, false);

  result += "\n\u3010 STATISTICS \u3011\n";
  result += std::format("  {}: {} attacks, {:.1f}% accuracy\n", winner_name,
                        winner_attacks, winner_accuracy * 100.0f);
  result += std::format("  {}: {} attacks, {:.1f}% accuracy\n", loser_name,
                        loser_attacks, loser_accuracy * 100.0f);
  result += "\n";

  return result;
}

std::string Renderer::render_game_start(std::string_view first_player) {
  std::string result = "\n=== BATTLESHIP GAME STARTED ===\n";
  result += std::format("{} goes first!\n", first_player);
  return result;
}

std::string Renderer::clear_screen() { return "\033[2J\033[1;1H"; }

// ConsoleRenderer
void ConsoleRenderer::display(std::string_view content) {
  std::cout << content;
}

void ConsoleRenderer::clear() { std::cout << "\033[2J\033[1;1H"; }

} // namespace battleship
