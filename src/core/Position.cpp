#include "Position.hpp"
#include <cctype>
#include <format>

namespace battleship {

Position::Position(std::string_view coords) {
  if (auto pos_opt = try_parse(coords); pos_opt) {
    *this = *pos_opt;
  } else {
    throw std::invalid_argument("Invalid coordinate format");
  }
}

std::optional<Position> Position::try_parse(std::string_view coords) noexcept {
  if (coords.length() < 2 || coords.length() > 3) {
    return std::nullopt;
  }

  const char letter = std::toupper(coords[0]);
  if (letter < 'A' || letter > 'J') {
    return std::nullopt;
  }

  const config::GridCoord x_coord = letter - 'A';

  const std::string_view number_str = coords.substr(1);
  int number = 0;

  // Manual parsing to avoid exceptions
  for (char c : number_str) {
    if (!std::isdigit(c)) {
      return std::nullopt;
    }
    number = number * 10 + (c - '0');
  }

  if (number < 1 || number > 10) {
    return std::nullopt;
  }

  const config::GridCoord y_coord = static_cast<config::GridCoord>(number - 1);

  Position pos{x_coord, y_coord};
  if (!pos.is_valid()) {
    return std::nullopt;
  }

  return pos;
}

std::string Position::to_string() const {
  return std::format("{}{}", static_cast<char>('A' + x), y + 1);
}

} // namespace battleship
