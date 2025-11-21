#include "Player.hpp"
#include "ShipManager.hpp"
#include <format>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>

namespace battleship {

Player::Player(std::string_view name, PlayerType type,
               config::Difficulty ai_difficulty)
    : m_name(name), m_type(type) {

  if (name.empty()) {
    throw std::invalid_argument("Player name cannot be empty");
  }

  if (m_type == PlayerType::AI) {
    m_ai_strategy = ai::make_strategy(ai_difficulty);
  }
}

bool Player::place_ship(config::ShipType type, const Position &pos,
                        Orientation orientation) {
  if (m_state != PlayerState::SETUP) {
    throw std::runtime_error("Cannot place ships after setup phase");
  }

  return m_board.place_ship(type, pos, orientation);
}

void Player::auto_place_ships() {
  if (m_state != PlayerState::SETUP) {
    throw std::runtime_error("Cannot auto-place ships after setup phase");
  }

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<config::GridCoord> dist(0,
                                                        Board::GRID_SIZE - 1);
  std::uniform_int_distribution<uint8_t> bool_dist(0, 1);

  for (const auto &ship_config : config::SHIP_CONFIGS) {
    for (uint8_t i = 0; i < ship_config.count; ++i) {
      bool placed = false;
      constexpr int MAX_ATTEMPTS = 1000;
      int attempts = 0;

      while (!placed && attempts < MAX_ATTEMPTS) {
        const config::GridCoord x = dist(gen);
        const config::GridCoord y = dist(gen);
        const Orientation orientation = bool_dist(gen) == 1
                                            ? Orientation::HORIZONTAL
                                            : Orientation::VERTICAL;

        const Position pos{x, y};
        placed = place_ship(ship_config.type, pos, orientation);
        ++attempts;
      }

      if (!placed) {
        throw std::runtime_error(std::format(
            "Failed to auto-place ship after {} attempts", MAX_ATTEMPTS));
      }
    }
  }

  m_state = PlayerState::READY;
}

void Player::manual_place_ships() {
  if (m_state != PlayerState::SETUP) {
    throw std::runtime_error("Cannot manually place ships after setup phase");
  }

  std::cout << "\n=== MANUAL SHIP PLACEMENT ===\n";
  std::cout << "Place your ships on the board.\n";
  std::cout << "Format: A1 H (for horizontal) or A1 V (for vertical)\n\n";

  for (const auto &ship_config : config::SHIP_CONFIGS) {
    for (uint8_t i = 0; i < ship_config.count; ++i) {
      std::cout << std::format("Placing {} ({}/{}), size {}\n",
                               ship_config.name, i + 1, ship_config.count,
                               ship_config.size());

      m_board.print(false);

      Position pos;
      Orientation orientation;

      while (!get_placement_from_user(ship_config.type, pos, orientation)) {
        std::cout << "Invalid placement! Try again.\n";
      }

      if (!place_ship(ship_config.type, pos, orientation)) {
        throw std::runtime_error("Failed to place ship after valid input");
      }

      std::cout << "Ship placed successfully!\n\n";
    }
  }

  m_state = PlayerState::READY;
  std::cout << "All ships placed! Ready for battle.\n";
}

Position Player::get_attack() {
  if (m_state != PlayerState::ACTIVE) {
    throw std::runtime_error("Player is not in attacking state");
  }

  return (m_type == PlayerType::HUMAN) ? get_human_attack() : get_ai_attack();
}

AttackResult Player::receive_attack(const Position &pos) {
  if (!pos.is_valid()) {
    throw std::invalid_argument("Invalid attack position");
  }

  return m_board.attack(pos);
}

void Player::record_attack_result(const Position &pos, AttackResult result) {
  m_attacked_positions.insert(pos);
  ++m_total_attacks;

  if (result == AttackResult::HIT || result == AttackResult::SUNK) {
    m_successful_hit_positions.emplace_back(pos);
    ++m_successful_hits_count;
  }

  if (m_ai_strategy) {
    m_ai_strategy->on_attack_result(pos, result);
  }
}

Position Player::get_human_attack() {
  std::string input;

  while (true) {
    std::cout << "Enter attack coordinates (e.g., A5): ";
    std::cin >> input;

    if (auto pos_opt = Position::try_parse(input); pos_opt) {
      if (is_valid_attack(*pos_opt)) {
        return *pos_opt;
      }
      std::cout << "Already attacked that position. Try again.\n";
    } else {
      std::cout << "Invalid input. Try again.\n";
    }
  }
}

Position Player::get_ai_attack() {
  if (!m_ai_strategy) {
    throw std::runtime_error("AI player has no strategy");
  }

  return m_ai_strategy->get_attack_position(m_attacked_positions,
                                            m_successful_hit_positions);
}

bool Player::is_valid_attack(const Position &pos) const noexcept {
  return pos.is_valid() && !m_attacked_positions.contains(pos);
}

bool Player::get_placement_from_user(config::ShipType type, Position &pos,
                                     Orientation &orientation) {
  std::string input;
  std::cout << "Enter position and orientation (e.g., A5 H): ";

  std::cin.clear();
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  if (!std::getline(std::cin, input)) {
    return false;
  }

  std::istringstream iss(input);
  std::string coord_str, orient_str;

  if (!(iss >> coord_str >> orient_str)) {
    std::cout << "Invalid format. Use: A1 H or A1 V\n";
    return false;
  }

  // Parse position
  if (auto pos_opt = Position::try_parse(coord_str); pos_opt) {
    pos = *pos_opt;
  } else {
    std::cout << "Invalid coordinates.\n";
    return false;
  }

  // Parse orientation
  const char orient = std::toupper(orient_str[0]);
  if (orient == 'H') {
    orientation = Orientation::HORIZONTAL;
  } else if (orient == 'V') {
    orientation = Orientation::VERTICAL;
  } else {
    std::cout << "Orientation must be H (horizontal) or V (vertical)\n";
    return false;
  }

  const auto size = ship_manager::get_ship_size(type);
  if (!m_board.can_place_ship(pos, size, orientation)) {
    std::cout << "Cannot place ship there. Invalid position or overlaps.\n";
    return false;
  }

  return true;
}

bool Player::all_ships_placed() const noexcept {
  return ship_manager::are_all_ships_placed(m_board.ships());
}

float Player::accuracy() const noexcept {
  if (m_total_attacks == 0) {
    return 0.0f;
  }
  return static_cast<float>(m_successful_hits_count) /
         static_cast<float>(m_total_attacks);
}

} // namespace battleship
