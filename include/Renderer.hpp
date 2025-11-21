#pragma once

#include "Board.hpp"
#include <string>
#include <vector>

namespace battleship {

struct TurnInfo;

class Renderer {
public:
  // Render the main game header
  static std::string render_header();

  // Render turn indicator
  static std::string render_turn(std::string_view player_name);

  // Render battle log (last N entries)
  static std::string render_battle_log(const std::vector<TurnInfo> &log,
                                       std::size_t max_entries = 3);

  // Render two boards side by side
  static std::string render_boards(const Board &left_board,
                                   const Board &right_board,
                                   std::string_view left_title,
                                   std::string_view right_title,
                                   bool hide_left_ships, bool hide_right_ships);

  // Render ship statistics
  static std::string render_statistics(const Board &player_board,
                                       const Board &opponent_board,
                                       std::string_view player_name,
                                       std::string_view opponent_name);

  // Render game over screen
  static std::string
  render_game_over(std::string_view winner_name, std::string_view loser_name,
                   const Board &winner_board, const Board &loser_board,
                   uint32_t winner_attacks, float winner_accuracy,
                   uint32_t loser_attacks, float loser_accuracy);

  // Clear screen escape sequence
  static std::string clear_screen();

private:
  static constexpr std::size_t BOX_WIDTH = 51;
  static constexpr std::size_t BOARD_WIDTH = 23;
  static constexpr std::size_t GAP_WIDTH = 7;
};

// Outputs rendered strings to console
class ConsoleRenderer {
public:
  static void display(const std::string &content);
  static void clear();
};

} // namespace battleship
