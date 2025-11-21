#pragma once

#include "Position.hpp"
#include <boost/asio.hpp>
#include <memory>
#include <optional>
#include <string>

namespace battleship::net {

using boost::asio::ip::tcp;

// Message types for protocol
enum class MessageType : uint8_t {
  ATTACK = 1,       // Position attack
  RESULT = 2,       // AttackResult response (1 byte)
  RESULT_SUNK = 3,  // Ship sunk with positions "A1,A2,A3"
  BOARD_STATE = 4,
  GAME_START = 5,
  GAME_OVER = 6,
  YOUR_TURN = 7,
  PING = 8,
  PONG = 9
};

// Simple message: type (1 byte) + length (2 bytes) + payload
struct Message {
  MessageType type;
  std::string payload;

  std::string serialize() const;
  static std::optional<Message> deserialize(const std::string &data);
};

class NetworkManager {
public:
  static constexpr uint16_t DEFAULT_PORT = 7777;

  NetworkManager();
  ~NetworkManager();

  // Host a game, wait for connection
  bool host(uint16_t port = DEFAULT_PORT);

  // Join a hosted game
  bool join(const std::string &host_ip, uint16_t port = DEFAULT_PORT);

  // Close connection
  void disconnect();

  // Check connection status
  bool is_connected() const noexcept { return m_connected; }
  bool is_host() const noexcept { return m_is_host; }

  // Send/receive messages
  bool send(const Message &msg);
  std::optional<Message> receive();

  // Convenience methods
  bool send_attack(const Position &pos);
  bool send_result(uint8_t result);
  bool send_board_state(const std::string &rendered_board);
  bool send_your_turn();
  bool send_game_over(const std::string &final_screen);

  std::optional<Position> receive_attack();
  std::optional<uint8_t> receive_result();

private:
  boost::asio::io_context m_io_context;
  std::unique_ptr<tcp::socket> m_socket;
  std::unique_ptr<tcp::acceptor> m_acceptor;

  bool m_connected{false};
  bool m_is_host{false};

  bool send_raw(const std::string &data);
  std::optional<std::string> receive_raw(std::size_t length);
};

} // namespace battleship::net
