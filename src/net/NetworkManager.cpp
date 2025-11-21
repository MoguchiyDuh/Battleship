#include "net/NetworkManager.hpp"
#include <iostream>

namespace battleship::net {

// Message serialization: [type:1][length:2][payload:N]
std::string Message::serialize() const {
  std::string result;
  result.reserve(3 + payload.size());

  result += static_cast<char>(type);

  const uint16_t len = static_cast<uint16_t>(payload.size());
  result += static_cast<char>(len >> 8);
  result += static_cast<char>(len & 0xFF);

  result += payload;
  return result;
}

std::optional<Message> Message::deserialize(const std::string &data) {
  if (data.size() < 3) {
    return std::nullopt;
  }

  Message msg;
  msg.type = static_cast<MessageType>(static_cast<uint8_t>(data[0]));

  const uint16_t len =
      (static_cast<uint8_t>(data[1]) << 8) | static_cast<uint8_t>(data[2]);

  if (data.size() < 3 + static_cast<std::size_t>(len)) {
    return std::nullopt;
  }

  msg.payload = data.substr(3, len);
  return msg;
}

NetworkManager::NetworkManager() = default;

NetworkManager::~NetworkManager() { disconnect(); }

bool NetworkManager::host(uint16_t port) {
  try {
    m_acceptor = std::make_unique<tcp::acceptor>(
        m_io_context, tcp::endpoint(tcp::v4(), port));

    std::cout << "Waiting for opponent on port " << port << "...\n";

    m_socket = std::make_unique<tcp::socket>(m_io_context);
    m_acceptor->accept(*m_socket);

    m_connected = true;
    m_is_host = true;

    std::cout << "Opponent connected!\n";
    return true;

  } catch (const std::exception &e) {
    std::cerr << "Host error: " << e.what() << "\n";
    return false;
  }
}

bool NetworkManager::join(const std::string &host_ip, uint16_t port) {
  try {
    m_socket = std::make_unique<tcp::socket>(m_io_context);

    tcp::resolver resolver(m_io_context);
    auto endpoints = resolver.resolve(host_ip, std::to_string(port));

    std::cout << "Connecting to " << host_ip << ":" << port << "...\n";

    boost::asio::connect(*m_socket, endpoints);

    m_connected = true;
    m_is_host = false;

    std::cout << "Connected to host!\n";
    return true;

  } catch (const std::exception &e) {
    std::cerr << "Join error: " << e.what() << "\n";
    return false;
  }
}

void NetworkManager::disconnect() {
  if (m_socket && m_socket->is_open()) {
    boost::system::error_code ec;
    m_socket->shutdown(tcp::socket::shutdown_both, ec);
    m_socket->close(ec);
  }
  m_socket.reset();
  m_acceptor.reset();
  m_connected = false;
}

bool NetworkManager::send(const Message &msg) {
  return send_raw(msg.serialize());
}

std::optional<Message> NetworkManager::receive() {
  // Read header (3 bytes)
  auto header = receive_raw(3);
  if (!header) {
    return std::nullopt;
  }

  const uint16_t payload_len = (static_cast<uint8_t>((*header)[1]) << 8) |
                               static_cast<uint8_t>((*header)[2]);

  // Read payload
  std::string full_msg = *header;
  if (payload_len > 0) {
    auto payload = receive_raw(payload_len);
    if (!payload) {
      return std::nullopt;
    }
    full_msg += *payload;
  }

  return Message::deserialize(full_msg);
}

bool NetworkManager::send_attack(const Position &pos) {
  Message msg;
  msg.type = MessageType::ATTACK;
  msg.payload = pos.to_string();
  return send(msg);
}

bool NetworkManager::send_result(uint8_t result) {
  Message msg;
  msg.type = MessageType::RESULT;
  msg.payload = std::string(1, static_cast<char>(result));
  return send(msg);
}

bool NetworkManager::send_board_state(const std::string &rendered_board) {
  Message msg;
  msg.type = MessageType::BOARD_STATE;
  msg.payload = rendered_board;
  return send(msg);
}

bool NetworkManager::send_your_turn() {
  Message msg;
  msg.type = MessageType::YOUR_TURN;
  return send(msg);
}

bool NetworkManager::send_game_over(const std::string &final_screen) {
  Message msg;
  msg.type = MessageType::GAME_OVER;
  msg.payload = final_screen;
  return send(msg);
}

std::optional<Position> NetworkManager::receive_attack() {
  auto msg = receive();
  if (!msg || msg->type != MessageType::ATTACK) {
    return std::nullopt;
  }
  return Position::try_parse(msg->payload);
}

std::optional<uint8_t> NetworkManager::receive_result() {
  auto msg = receive();
  if (!msg || msg->type != MessageType::RESULT || msg->payload.empty()) {
    return std::nullopt;
  }
  return static_cast<uint8_t>(msg->payload[0]);
}

bool NetworkManager::send_raw(const std::string &data) {
  if (!m_connected || !m_socket) {
    return false;
  }

  try {
    boost::asio::write(*m_socket, boost::asio::buffer(data));
    return true;
  } catch (const std::exception &e) {
    std::cerr << "Send error: " << e.what() << "\n";
    m_connected = false;
    return false;
  }
}

std::optional<std::string> NetworkManager::receive_raw(std::size_t length) {
  if (!m_connected || !m_socket) {
    return std::nullopt;
  }

  try {
    std::string buffer(length, '\0');
    boost::asio::read(*m_socket, boost::asio::buffer(buffer));
    return buffer;
  } catch (const std::exception &e) {
    std::cerr << "Receive error: " << e.what() << "\n";
    m_connected = false;
    return std::nullopt;
  }
}

} // namespace battleship::net
