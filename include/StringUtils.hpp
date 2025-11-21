#pragma once

#include <string>
#include <string_view>

namespace battleship::str {

// repeat("=", 10) -> "=========="
inline std::string repeat(std::string_view s, std::size_t n) {
  std::string result;
  result.reserve(s.size() * n);
  for (std::size_t i = 0; i < n; ++i) {
    result.append(s);
  }
  return result;
}

// center("hello", 20) -> "       hello        "
inline std::string center(std::string_view text, std::size_t width,
                          char fill = ' ') {
  if (text.size() >= width) {
    return std::string(text);
  }
  std::string result;
  result.reserve(width);
  const std::size_t padding = width - text.size();
  const std::size_t left = padding / 2;
  const std::size_t right = padding - left;
  result.append(left, fill);
  result.append(text);
  result.append(right, fill);
  return result;
}

// ljust("hello", 10) -> "hello     "
inline std::string ljust(std::string_view text, std::size_t width,
                         char fill = ' ') {
  if (text.size() >= width) {
    return std::string(text);
  }
  std::string result;
  result.reserve(width);
  result.append(text);
  result.append(width - text.size(), fill);
  return result;
}

// rjust("hello", 10) -> "     hello"
inline std::string rjust(std::string_view text, std::size_t width,
                         char fill = ' ') {
  if (text.size() >= width) {
    return std::string(text);
  }
  std::string result;
  result.reserve(width);
  result.append(width - text.size(), fill);
  result.append(text);
  return result;
}

} // namespace battleship::str
