#include "Game.hpp"
#include <format>
#include <iostream>
#include <limits>

using namespace battleship;

void print_menu() {
  std::cout << "\n=== BATTLESHIP ===\n";
  std::cout << "Select game mode:\n";
  std::cout << "  1. Player vs Player\n";
  std::cout << "  2. Player vs Computer (Easy)\n";
  std::cout << "  3. Player vs Computer (Medium)\n";
  std::cout << "  4. Player vs Computer (Hard)\n";
  std::cout << "  5. Computer vs Computer (Watch)\n";
  std::cout << "  0. Exit\n";
  std::cout << "\nChoice: ";
}

[[nodiscard]] std::optional<GameMode> get_game_mode() {
  int choice;
  std::cin >> choice;

  if (std::cin.fail()) {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return std::nullopt;
  }

  switch (choice) {
  case 1:
    return GameMode::PVP;
  case 2:
    return GameMode::PVE_EASY;
  case 3:
    return GameMode::PVE_MEDIUM;
  case 4:
    return GameMode::PVE_HARD;
  case 5:
    return GameMode::AI_VS_AI;
  case 0:
    return std::nullopt;
  default:
    return std::nullopt;
  }
}

int main() {
  try {
    while (true) {
      print_menu();
      const auto mode_opt = get_game_mode();

      if (!mode_opt) {
        std::cout << "Thanks for playing!\n";
        break;
      }

      Game game(*mode_opt);
      game.initialize();
      game.start();

      while (!game.is_game_over()) {
        game.run_turn();
      }

      std::cout << "\nPlay again? (y/n): ";
      char response;
      std::cin >> response;

      if (std::tolower(response) != 'y') {
        std::cout << "Thanks for playing!\n";
        break;
      }
    }

    return 0;
  } catch (const std::exception &e) {
    std::cerr << std::format("Fatal error: {}\n", e.what());
    return 1;
  }
}
