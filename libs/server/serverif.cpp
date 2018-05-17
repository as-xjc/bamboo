#include <bamboo/server/serverif.hpp>

namespace bamboo {
namespace server {

ServerIf::ServerIf(boost::asio::io_context& io, std::string name, std::size_t index) : io_(io),
                                                                                       name_(std::move(name)),
                                                                                       ioIndex_(index) {

  scheduler_.reset(new bamboo::schedule::Scheduler(io));
}

ServerIf::~ServerIf() {

}

const std::string& ServerIf::GetName() const {
  return name_;
}

bool ServerIf::Start() {
  if (!PrepareStart()) return false;

  try {
    for (auto& it : acceptors_) {
      it->Start();
    }
  } catch (std::exception& e) {
    BB_ERROR_LOG("server[%s] start fail:%s", GetName().c_str(), e.what());
    return false;
  }

  return FinishStart();
}

void ServerIf::Stop() {
  for (auto& it : acceptors_) {
    it->Stop();
  }

  StopHandle();
}

std::size_t ServerIf::GetIoIndex() const {
  return ioIndex_;
}

bamboo::schedule::Scheduler& ServerIf::GetScheduler() {
  return *scheduler_.get();
}

bamboo::net::AcceptorPtr ServerIf::GetAcceptor(uint32_t index) {
  return acceptors_[index];
}

bamboo::net::ConnectorPtr ServerIf::GetConnector(uint32_t index) {
  return connectors_[index];
}

}
}