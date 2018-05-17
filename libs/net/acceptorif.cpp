#include "bamboo/net/acceptorif.hpp"

namespace bamboo {
namespace net {

AcceptorIf::AcceptorIf(boost::asio::io_context& io) : io_(io) {}

AcceptorIf::~AcceptorIf() {}

void AcceptorIf::SetAddress(std::string address, uint16_t port) {
  BB_ASSERT(acceptor_ == nullptr);

  auto end = boost::asio::ip::make_address(address);
  boost::asio::ip::tcp::endpoint endpoint(end, port);
  acceptor_.reset(new boost::asio::ip::tcp::acceptor(io_, endpoint));
}

ConnManagerPtr AcceptorIf::GetConnManager() {
  return connManager_;
}

void AcceptorIf::SetConnManager(ConnManagerPtr& mgr) {
  BB_ASSERT(connManager_ == nullptr && mgr != nullptr);
  connManager_ = mgr;
}

}
}