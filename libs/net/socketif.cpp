#include "bamboo/net/socketif.hpp"

namespace bamboo {
namespace net {

SocketIf::SocketIf(boost::asio::ip::tcp::socket&& socket) :
    boost::asio::ip::tcp::socket(std::move(socket)) {}

SocketIf::~SocketIf() {}

uint32_t SocketIf::GetId() {
  return id_;
}

void SocketIf::SetId(uint32_t id) {
  id_ = id;
}

void SocketIf::SetReadHandler(bamboo::net::SocketIf::ReadHandler&& handle) {
  reader_ = std::move(handle);
}

SocketIf::ReadHandler& SocketIf::GetReadHandler() {
  return reader_;
}

void SocketIf::SetCloseHandler(bamboo::net::SocketIf::CloseHandler&& handle) {
  closer_ = std::move(handle);
}

SocketIf::CloseHandler& SocketIf::GetCloseHandler() {
  return closer_;
}

}
}