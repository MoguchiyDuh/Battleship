#pragma once

#include "Config.hpp"
#include "Ship.hpp"
#include <string_view>
#include <vector>

namespace battleship::ship_manager {

inline std::string_view get_ship_name(config::ShipType type) noexcept {
  for (const auto &cfg : config::SHIP_CONFIGS) {
    if (cfg.type == type) {
      return cfg.name;
    }
  }
  return "Unknown";
}

inline uint8_t get_ship_count(config::ShipType type) noexcept {
  for (const auto &cfg : config::SHIP_CONFIGS) {
    if (cfg.type == type) {
      return cfg.count;
    }
  }
  return 0;
}

inline config::GridSize get_ship_size(config::ShipType type) noexcept {
  return static_cast<config::GridSize>(type);
}

inline bool are_all_ships_placed(const std::vector<std::unique_ptr<Ship>> &ships) noexcept {
  std::array<uint8_t, config::SHIP_CONFIGS.size()> counts{};

  for (const auto &ship : ships) {
    for (std::size_t i = 0; i < config::SHIP_CONFIGS.size(); ++i) {
      if (ship->type() == config::SHIP_CONFIGS[i].type) {
        ++counts[i];
        break;
      }
    }
  }

  for (std::size_t i = 0; i < config::SHIP_CONFIGS.size(); ++i) {
    if (counts[i] != config::SHIP_CONFIGS[i].count) {
      return false;
    }
  }

  return true;
}

} // namespace battleship::ship_manager
