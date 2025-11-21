#pragma once

#include "Board.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace battleship {

struct TurnInfo;

// All rendering/display logic consolidated here
class Renderer {
public:
  // Core rendering methods - return formatted strings
  static std::string render_header();
  static std::string render_turn(std::string_view player_name);
  static std::string render_battle_log(const std::vector<TurnInfo> &log,
                                       std::size_t max_entries = 3);
  static std::string render_boards(const Board &left_board,
                                   const Board &right_board,
                                   std::string_view left_title,
                                   std::string_view right_title,
                                   bool hide_left_ships, bool hide_right_ships);
  static std::string render_statistics(const Board &player_board,
                                       const Board &opponent_board,
                                       std::string_view player_name,
                                       std::string_view opponent_name);
  static std::string render_statistics(const Board::ShipTypeCounts &player_counts,
                                       uint8_t player_total,
                                       const Board::ShipTypeCounts &opponent_counts,
                                       uint8_t opponent_total,
                                       std::string_view player_name,
                                       std::string_view opponent_name);
  static std::string render_game_over(std::string_view winner_name,
                                      std::string_view loser_name,
                                      const Board &winner_board,
                                      const Board &loser_board,
                                      uint32_t winner_attacks, float winner_accuracy,
                                      uint32_t loser_attacks, float loser_accuracy);
  static std::string render_game_start(std::string_view first_player);

  // ANSI escape sequences
  static std::string clear_screen();

private:
  static constexpr std::size_t BOX_WIDTH = 51;
  static constexpr std::size_t BOARD_WIDTH = 23;
  static constexpr std::size_t GAP_WIDTH = 7;

  static std::string render_single_board(const Board::DisplayGrid &grid);
  static std::string result_to_string(AttackResult result);
};

// Console output - thin wrapper over cout
class ConsoleRenderer {
public:
  static void display(std::string_view content);
  static void clear();
};

} // namespace battleship
