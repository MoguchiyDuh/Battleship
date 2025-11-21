#include "Game.hpp"
#include "OnlineGame.hpp"
#include "net/NetworkManager.hpp"
#include <format>
#include <iostream>
#include <limits>

using namespace battleship;

void print_menu() {
  std::cout << "\n=== BATTLESHIP ===\n";
  std::cout << "Select game mode:\n";
  std::cout << "  1. Player vs Player (Local)\n";
  std::cout << "  2. Player vs Player (Online - Host)\n";
  std::cout << "  3. Player vs Player (Online - Join)\n";
  std::cout << "  4. Player vs Computer (Easy)\n";
  std::cout << "  5. Player vs Computer (Medium)\n";
  std::cout << "  6. Player vs Computer (Hard)\n";
  std::cout << "  7. Computer vs Computer (Watch)\n";
  std::cout << "  0. Exit\n";
  std::cout << "\nChoice: ";
}

void run_online_host() {
  net::NetworkManager network;
  if (!network.host()) {
    std::cerr << "Failed to start hosting\n";
    return;
  }

  OnlineGame game(network);
  game.initialize();
  game.run();
}

void run_online_join() {
  std::string host_ip;
  std::cout << "Enter host IP: ";
  std::cin >> host_ip;

  net::NetworkManager network;
  if (!network.join(host_ip)) {
    std::cerr << "Failed to connect to host\n";
    return;
  }

  OnlineGame game(network);
  game.initialize();
  game.run();
}

// Returns: -1 = exit, 0 = online host, 1 = online join, 2+ = GameMode
[[nodiscard]] int get_menu_choice() {
  int choice;
  std::cin >> choice;

  if (std::cin.fail()) {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return -1;
  }

  return choice;
}

void run_local_game(GameMode mode) {
  Game game(mode);
  game.initialize();
  game.start();

  while (!game.is_game_over()) {
    game.run_turn();
  }
}

int main() {
  try {
    while (true) {
      print_menu();
      const int choice = get_menu_choice();

      switch (choice) {
      case 0:
        std::cout << "Thanks for playing!\n";
        return 0;
      case 1:
        run_local_game(GameMode::PVP);
        break;
      case 2:
        run_online_host();
        break;
      case 3:
        run_online_join();
        break;
      case 4:
        run_local_game(GameMode::PVE_EASY);
        break;
      case 5:
        run_local_game(GameMode::PVE_MEDIUM);
        break;
      case 6:
        run_local_game(GameMode::PVE_HARD);
        break;
      case 7:
        run_local_game(GameMode::AI_VS_AI);
        break;
      default:
        std::cout << "Invalid choice\n";
        continue;
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
