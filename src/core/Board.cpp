#include "Board.hpp"
#include <algorithm>
#include <format>
#include <iostream>

namespace battleship {

Board::Board() {
  clear();
  initialize_ship_lookup();
}

void Board::initialize_ship_lookup() noexcept {
  for (auto &row : m_ship_lookup) {
    std::ranges::fill(row, nullptr);
  }
}

void Board::clear() noexcept {
  for (auto &row : m_grid) {
    std::ranges::fill(row, CellState::EMPTY);
  }
  m_ships.clear();
  m_attacked_positions.clear();
  m_total_attacks = 0;
  m_successful_hits = 0;
  initialize_ship_lookup();
}

bool Board::is_valid_position(const Position &pos) const noexcept {
  return pos.is_valid();
}

bool Board::can_place_ship(const Position &pos, config::GridSize size,
                           Orientation orientation) const noexcept {
  if (!is_valid_position(pos)) {
    return false;
  }

  // Check boundaries
  if (orientation == Orientation::HORIZONTAL) {
    if (pos.x + size > GRID_SIZE) {
      return false;
    }
  } else {
    if (pos.y + size > GRID_SIZE) {
      return false;
    }
  }

  // Check all positions are clear
  for (uint8_t i = 0; i < size; ++i) {
    const Position check_pos =
        (orientation == Orientation::HORIZONTAL)
            ? Position{static_cast<config::GridCoord>(pos.x + i), pos.y}
            : Position{pos.x, static_cast<config::GridCoord>(pos.y + i)};

    if (!is_valid_position(check_pos)) {
      return false;
    }

    if (m_grid[check_pos.y][check_pos.x] != CellState::EMPTY) {
      return false;
    }

    if (!is_area_clear(check_pos)) {
      return false;
    }
  }

  return true;
}

bool Board::is_area_clear(const Position &pos) const noexcept {
  for (const auto &[dx, dy] : config::ALL_DIRECTIONS) {
    const int check_x = pos.x + dx;
    const int check_y = pos.y + dy;

    if (check_x < 0 || check_x >= GRID_SIZE || check_y < 0 ||
        check_y >= GRID_SIZE) {
      continue;
    }

    if (m_grid[check_y][check_x] == CellState::SHIP) {
      return false;
    }
  }

  return true;
}

bool Board::place_ship(config::ShipType type, const Position &pos,
                       Orientation orientation) {
  if (!is_valid_position(pos)) {
    throw std::invalid_argument("Invalid position for ship placement");
  }

  const auto size = static_cast<config::GridSize>(type);
  if (!can_place_ship(pos, size, orientation)) {
    return false;
  }

  auto ship = std::make_unique<Ship>(type, pos, orientation);
  const auto positions = ship->positions();

  for (const auto &ship_pos : positions) {
    if (!is_valid_position(ship_pos)) {
      throw std::runtime_error("Ship placement generated invalid position");
    }
    m_grid[ship_pos.y][ship_pos.x] = CellState::SHIP;
    m_ship_lookup[ship_pos.y][ship_pos.x] = ship.get();
  }

  m_ships.push_back(std::move(ship));

  return true;
}

AttackResult Board::attack(const Position &pos) {
  if (!is_valid_position(pos)) {
    return AttackResult::INVALID_COORD;
  }

  // Fast duplicate check using hash set
  if (m_attacked_positions.contains(pos)) {
    return AttackResult::ALREADY_ATTACKED;
  }

  m_attacked_positions.insert(pos);
  ++m_total_attacks;

  CellState &cell = m_grid[pos.y][pos.x];

  if (cell == CellState::SHIP) {
    Ship *ship = m_ship_lookup[pos.y][pos.x];
    if (!ship) {
      throw std::runtime_error("Grid shows SHIP but no ship found at position");
    }

    const bool was_hit = ship->register_hit(pos);
    if (!was_hit) {
      cell = CellState::MISS;
      return AttackResult::MISS;
    }

    cell = CellState::HIT;
    ++m_successful_hits;

    if (ship->is_sunk()) {
      update_sunk_ship_cells(*ship);
      return AttackResult::SUNK;
    }
    return AttackResult::HIT;
  }

  cell = CellState::MISS;
  return AttackResult::MISS;
}

void Board::mark_attack(const Position &pos, AttackResult result) {
  if (!is_valid_position(pos)) {
    return;
  }

  m_attacked_positions.insert(pos);
  CellState &cell = m_grid[pos.y][pos.x];

  switch (result) {
  case AttackResult::MISS:
    cell = CellState::MISS;
    break;
  case AttackResult::HIT:
    cell = CellState::HIT;
    break;
  case AttackResult::SUNK:
    cell = CellState::SUNK;
    break;
  default:
    break;
  }
}

void Board::mark_sunk_ship(const std::vector<Position> &ship_cells) {
  // Mark all ship cells as SUNK
  for (const auto &pos : ship_cells) {
    if (is_valid_position(pos)) {
      m_grid[pos.y][pos.x] = CellState::SUNK;
      m_attacked_positions.insert(pos);
    }
  }

  // Mark surrounding cells as MISS
  for (const auto &pos : ship_cells) {
    for (int dy = -1; dy <= 1; ++dy) {
      for (int dx = -1; dx <= 1; ++dx) {
        if (dx == 0 && dy == 0)
          continue;

        const int nx = pos.x + dx;
        const int ny = pos.y + dy;

        if (nx >= 0 && nx < GRID_SIZE && ny >= 0 && ny < GRID_SIZE) {
          Position neighbor{static_cast<config::GridCoord>(nx),
                            static_cast<config::GridCoord>(ny)};
          if (m_grid[ny][nx] == CellState::EMPTY) {
            m_grid[ny][nx] = CellState::MISS;
            m_attacked_positions.insert(neighbor);
          }
        }
      }
    }
  }
}

const Ship *Board::get_ship_at(const Position &pos) const noexcept {
  if (!is_valid_position(pos)) {
    return nullptr;
  }
  return m_ship_lookup[pos.y][pos.x];
}

void Board::update_sunk_ship_cells(const Ship &ship) noexcept {
  for (const auto &pos : ship.positions()) {
    if (!is_valid_position(pos)) {
      continue;
    }
    m_grid[pos.y][pos.x] = CellState::SUNK;
  }

  for (const auto &pos : ship.positions()) {
    mark_surrounding_cells_as_miss(pos);
  }
}

void Board::mark_surrounding_cells_as_miss(const Position &pos) noexcept {
  for (const auto &[dx, dy] : config::ALL_DIRECTIONS) {
    const int check_x = pos.x + dx;
    const int check_y = pos.y + dy;

    if (check_x < 0 || check_x >= GRID_SIZE || check_y < 0 ||
        check_y >= GRID_SIZE) {
      continue;
    }

    // Only mark EMPTY cells as MISS, don't touch SHIP/HIT/SUNK cells
    if (m_grid[check_y][check_x] == CellState::EMPTY) {
      m_grid[check_y][check_x] = CellState::MISS;
    }
  }
}

bool Board::is_game_over() const noexcept {
  return std::ranges::all_of(m_ships,
                             [](const auto &ship) { return ship->is_sunk(); });
}

CellState Board::get_cell_state(const Position &pos) const {
  if (!is_valid_position(pos)) {
    throw std::invalid_argument("Invalid position for cell state lookup");
  }
  return m_grid[pos.y][pos.x];
}

char Board::get_cell_symbol(CellState state, bool show_ships) const noexcept {
  const auto index = static_cast<std::size_t>(state);
  return show_ships ? SHOWN_SYMBOLS[index] : HIDDEN_SYMBOLS[index];
}

Board::DisplayGrid Board::render(bool hide_ships) const {
  DisplayGrid display_grid;

  for (uint8_t y = 0; y < GRID_SIZE; ++y) {
    for (uint8_t x = 0; x < GRID_SIZE; ++x) {
      const CellState state = m_grid[y][x];
      display_grid[y][x] = get_cell_symbol(state, !hide_ships);
    }
  }

  return display_grid;
}

void Board::print(bool hide_ships) const {
  const DisplayGrid grid = render(hide_ships);

  std::cout << "  A B C D E F G H I J\n";
  for (uint8_t y = 0; y < GRID_SIZE; ++y) {
    std::cout << std::format("{:2} ", y + 1);
    for (uint8_t x = 0; x < GRID_SIZE; ++x) {
      std::cout << grid[y][x] << ' ';
    }
    std::cout << '\n';
  }
}

uint8_t Board::ships_remaining() const noexcept {
  return static_cast<uint8_t>(std::ranges::count_if(
      m_ships, [](const auto &ship) { return !ship->is_sunk(); }));
}

uint8_t Board::ships_sunk() const noexcept {
  return static_cast<uint8_t>(std::ranges::count_if(
      m_ships, [](const auto &ship) { return ship->is_sunk(); }));
}

Board::ShipTypeCounts Board::get_remaining_ship_types() const noexcept {
  ShipTypeCounts counts;

  for (const auto &ship : m_ships) {
    if (ship->is_sunk()) {
      continue;
    }

    switch (ship->type()) {
    case config::ShipType::BATTLESHIP:
      ++counts.battleships;
      break;
    case config::ShipType::CRUISER:
      ++counts.cruisers;
      break;
    case config::ShipType::DESTROYER:
      ++counts.destroyers;
      break;
    case config::ShipType::PATROL_BOAT:
      ++counts.patrol_boats;
      break;
    }
  }

  return counts;
}

} // namespace battleship
