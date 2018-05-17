#include "bamboo/net/simpleconnmanager.hpp"

namespace bamboo {
namespace net {

SimpleConnManager::~SimpleConnManager() {}

SocketPtr SimpleConnManager::OnConnect(boost::asio::ip::tcp::socket&& socket) {
  auto so = std::make_shared<bamboo::net::Socket>(std::move(socket));
  auto id = static_cast<uint32_t>(so->native_handle());
  so->SetId(id);
  sockets_[so->GetId()] = so;

  auto self = shared_from_this();
  so->SetReadHandler([id, self, this](const char* data, std::size_t size) -> std::size_t {
    auto& reader = GetReadHandler();
    if (reader) {
      auto it = sockets_.find(id);
      if (it == sockets_.end()) return size;
      return reader(it->second, data, size);
    }
    return size;
  });

  so->SetCloseHandler([self, id]() { self->OnDelete(id); });

  auto& connector = GetConnectHandler();
  if (connector) connector(std::dynamic_pointer_cast<SocketIf>(so));

  so->ReadData();
  return so;
}

void SimpleConnManager::OnDelete(uint32_t socket) {
  auto it = sockets_.find(socket);
  if (it == sockets_.end()) return;

  auto& deleter = GetDeleteHandler();
  if (deleter) deleter(it->second);

  sockets_.erase(it);
}

void SimpleConnManager::WriteData(uint32_t socket, const char* data, std::size_t size) {
  auto it = sockets_.find(socket);
  if (it == sockets_.end()) return;

  it->second->WriteData(data, size);
}

}
}