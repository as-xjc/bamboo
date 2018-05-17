#include "bamboo/net/connectorif.hpp"
#include "bamboo/log/log.hpp"

namespace bamboo {
namespace net {

ConnectorIf::ConnectorIf(boost::asio::io_context &io) : io_(io) {}

ConnManagerPtr ConnectorIf::GetConnManager() {
  return connManager_;
}

void ConnectorIf::SetConnManager(ConnManagerPtr& mgr) {
  BB_ASSERT(connManager_ == nullptr && mgr != nullptr);
  connManager_ = mgr;
}

SocketPtr ConnectorIf::Connect(std::string address, uint16_t port) {
  boost::asio::ip::tcp::socket socket(io_);
  boost::asio::ip::tcp::resolver resolver(io_);
  boost::system::error_code ec;
  std::string _port = std::to_string(port);
  auto result = resolver.resolve(address, _port, ec);
  if (ec) {
    BB_ERROR_LOG("resolver address[%s] fail:%s", address.c_str(), ec.message().c_str());
    return nullptr;
  }
  boost::asio::connect(socket, result, ec);
  if (ec) {
    BB_ERROR_LOG("connect to [%s:%d] fail:%s", address.c_str(), port, ec.message().c_str());
    return nullptr;
  }

  if (connManager_) {
    BB_INFO_LOG("established connection to [%s:%d]",
                  socket.remote_endpoint().address().to_string().c_str(), port);
    return connManager_->OnConnect(std::move(socket));
  }

  BB_ERROR_LOG("established connection to [%s:%d] but no conn manager.",
                socket.remote_endpoint().address().to_string().c_str(), port);

  return nullptr;
}

}
}