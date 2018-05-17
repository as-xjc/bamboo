#include "bamboo/net/simpleacceptor.hpp"

namespace bamboo {
namespace net {

SimpleAcceptor::~SimpleAcceptor() {}

void SimpleAcceptor::Start() {
  if (acceptor_) {
    DoAccept();
  }
}

void SimpleAcceptor::Stop() {
  acceptor_->close();
}

void SimpleAcceptor::DoAccept() {
  auto self = shared_from_this();
  acceptor_->async_accept(
      [this, self](boost::system::error_code ec, boost::asio::ip::tcp::socket&& socket) {
        if (ec) {
          BB_ERROR_LOG("accept connect fail:%s", ec.message().c_str());
        } else {
          if (connManager_) {
            connManager_->OnConnect(std::move(socket));
          }
        }
        DoAccept();
      });
}

}
}