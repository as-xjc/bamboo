#include "bamboo/aio/aioif.hpp"

namespace bamboo {
namespace aio {

AioIf::AioIf() {}

AioIf::~AioIf() {}

void AioIf::Start() {
  if (registry_) {
    registry_->InitServerId();
    registry_->Connect();
  }

  for (auto& it : servers_) {
    bool success = it->Start();
    if (!success) {
      throw std::runtime_error("start server fail:" + it->GetName());
    }
  }

  if (registry_) {
    registry_->Register();
    registry_->StartWatch();
  }
  IoRun();
}

void AioIf::Stop() {
  if (registry_) registry_->Stop();

  for (auto& server : servers_) {
    server->Stop();
  }

  StopHandle();
}

std::shared_ptr<bamboo::distributed::Registry> AioIf::InitRegistry() {
  BB_ASSERT(registry_ == nullptr);
  registry_.reset(new bamboo::distributed::Registry(GetMasterIo()));
  return registry_;
}

std::shared_ptr<bamboo::distributed::Registry> AioIf::GetRegistry() {
  return registry_;
}

void AioIf::Signal(int signal, std::function<void(int)>&& handler) {
  auto ptr = std::make_shared<SignalInfo>(GetMasterIo(), signal);
  ptr->handler = [handler, ptr](const boost::system::error_code& error, int signal) {
    if (error) {
      BB_ERROR_LOG("catch signal[%d] fail:%s", signal, error.message().c_str());
    } else {
      handler(signal);
      ptr->signal.async_wait(ptr->handler);
    }
  };
  ptr->signal.async_wait(ptr->handler);
  signals_.insert(std::make_pair(signal, ptr));
}

void AioIf::Configure(boost::program_options::variables_map& vm) {
  for (auto& it : servers_) {
    it->Configure(vm);
  }
}

}
}