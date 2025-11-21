#pragma once

#include "Config.hpp"
#include "Ship.hpp"
#include <array>
#include <memory>
#include <unordered_set>
#include <vector>

namespace battleship {

enum class CellState : uint8_t { EMPTY, SHIP, HIT, MISS, SUNK };

enum class AttackResult : uint8_t {
  MISS,
  HIT,
  SUNK,
  ALREADY_ATTACKED,
  INVALID_COORD
};

class Board {
public:
  static constexpr config::GridSize GRID_SIZE = config::GRID_SIZE;
  using Grid = std::array<std::array<CellState, GRID_SIZE>, GRID_SIZE>;
  using DisplayGrid = std::array<std::array<char, GRID_SIZE>, GRID_SIZE>;
  using ShipLookup = std::array<std::array<Ship *, GRID_SIZE>, GRID_SIZE>;

  Board();

  bool place_ship(config::ShipType type, const Position &pos,
                  Orientation orientation);
  bool can_place_ship(const Position &pos, config::GridSize size,
                      Orientation orientation) const noexcept;

  AttackResult attack(const Position &pos);
  void mark_attack(const Position &pos, AttackResult result); // for tracking
  bool is_game_over() const noexcept;                         // all ships sunk

  CellState get_cell_state(const Position &pos) const;
  const Ship *get_ship_at(const Position &pos) const noexcept;

  const std::vector<std::unique_ptr<Ship>> &ships() const noexcept {
    return m_ships;
  }

  // hide_ships: true = show only hits/misses, false = show ship positions
  DisplayGrid render(bool hide_ships = false) const;
  void print(bool hide_ships = false) const;

  void clear() noexcept;

  uint8_t ships_remaining() const noexcept;
  uint8_t ships_sunk() const noexcept;

  struct ShipTypeCounts {
    uint8_t battleships{0};
    uint8_t cruisers{0};
    uint8_t destroyers{0};
    uint8_t patrol_boats{0};
  };
  ShipTypeCounts get_remaining_ship_types() const noexcept;

private:
  Grid m_grid{};
  ShipLookup m_ship_lookup{}; // fast O(1) ship lookup by position
  std::vector<std::unique_ptr<Ship>> m_ships;

  // O(1) duplicate attack detection
  std::unordered_set<Position, Position::Hash> m_attacked_positions;

  uint16_t m_total_attacks{0};
  uint16_t m_successful_hits{0};

  // Display symbols: EMPTY, SHIP, HIT, MISS, SUNK
  static constexpr std::array<char, 5> HIDDEN_SYMBOLS = {'~', '~', 'X', 'O',
                                                         '#'};
  static constexpr std::array<char, 5> SHOWN_SYMBOLS = {'~', 'S', 'X', 'O',
                                                        '#'};

  bool is_valid_position(const Position &pos) const noexcept;
  bool is_area_clear(const Position &pos) const noexcept; // no adjacent ships
  void update_sunk_ship_cells(const Ship &ship) noexcept;
  void mark_surrounding_cells_as_miss(const Position &pos) noexcept;
  char get_cell_symbol(CellState state, bool show_ships) const noexcept;

  void initialize_ship_lookup() noexcept;
};

} // namespace battleship
