#include "Renderer.hpp"
#include "Game.hpp"

namespace battleship {

std::string Renderer::render_header() {
  std::string result;
  result += "╔" + str::repeat("═", BOX_WIDTH) + "╗\n";
  result += "║" + str::center("BATTLESHIP", BOX_WIDTH) + "║\n";
  result += "╚" + str::repeat("═", BOX_WIDTH) + "╝\n\n";
  return result;
}

std::string Renderer::render_turn(std::string_view player_name) {
  return std::format("【 {}'s Turn 】\n\n", player_name);
}

std::string Renderer::render_battle_log(const std::vector<TurnInfo> &log,
                                        std::size_t max_entries) {
  if (log.empty()) {
    return "";
  }

  std::string result = "【 BATTLE LOG 】\n";
  const std::size_t start_idx =
      log.size() > max_entries ? log.size() - max_entries : 0;

  for (std::size_t i = start_idx; i < log.size(); ++i) {
    const auto &entry = log[i];
    result += std::format("  {} attacked {} → ", entry.attacker_name,
                          entry.attack_pos.to_string());

    switch (entry.result) {
    case AttackResult::MISS:
      result += "MISS";
      break;
    case AttackResult::HIT:
      result += "HIT!";
      break;
    case AttackResult::SUNK:
      result += "SUNK!";
      break;
    case AttackResult::ALREADY_ATTACKED:
      result += "Already attacked";
      break;
    case AttackResult::INVALID_COORD:
      result += "Invalid";
      break;
    }
    result += "\n";
  }
  result += "\n";
  return result;
}

std::string Renderer::render_boards(const Board &left_board,
                                    const Board &right_board,
                                    std::string_view left_title,
                                    std::string_view right_title,
                                    bool hide_left_ships,
                                    bool hide_right_ships) {
  std::string result;

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

  // Rows
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
  std::string result = "\n【 STATISTICS 】\n";

  const auto p1_counts = player_board.get_remaining_ship_types();
  const auto p2_counts = opponent_board.get_remaining_ship_types();
  const auto p1_total = player_board.ships_remaining();
  const auto p2_total = opponent_board.ships_remaining();

  result += std::format("  {}: {} ships (B:{} C:{} D:{} P:{})\n", player_name,
                        p1_total, p1_counts.battleships, p1_counts.cruisers,
                        p1_counts.destroyers, p1_counts.patrol_boats);
  result += std::format("  {}: {} ships (B:{} C:{} D:{} P:{})\n", opponent_name,
                        p2_total, p2_counts.battleships, p2_counts.cruisers,
                        p2_counts.destroyers, p2_counts.patrol_boats);
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

  result += "【 GAME OVER 】\n\n";
  result += std::format("  {} WINS!\n\n", winner_name);

  result += "【 FINAL BOARDS 】\n";
  result += render_boards(winner_board, loser_board, winner_name, loser_name,
                          false, false);

  result += "\n【 STATISTICS 】\n";
  result += std::format("  {}: {} attacks, {:.1f}% accuracy\n", winner_name,
                        winner_attacks, winner_accuracy * 100.0f);
  result += std::format("  {}: {} attacks, {:.1f}% accuracy\n", loser_name,
                        loser_attacks, loser_accuracy * 100.0f);
  result += "\n";

  return result;
}

std::string Renderer::clear_screen() { return "\033[2J\033[1;1H"; }

// ConsoleRenderer
void ConsoleRenderer::display(const std::string &content) {
  std::cout << content;
}

void ConsoleRenderer::clear() { std::cout << "\033[2J\033[1;1H"; }

} // namespace battleship
