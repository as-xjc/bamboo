#include "bamboo/net/connmanagerif.hpp"

namespace bamboo {
namespace net {

ConnManagerIf::ConnManagerIf() {}

ConnManagerIf::~ConnManagerIf() {}

void ConnManagerIf::SetReadHandler(ConnManagerIf::ReadHandler&& handle) {
  BB_ASSERT(!protocol_);
  BB_ASSERT(!reader_);
  reader_ = std::move(handle);
}

ConnManagerIf::ReadHandler& ConnManagerIf::GetReadHandler() {
  return reader_;
}

void ConnManagerIf::SetDeleteHandler(bamboo::net::ConnManagerIf::DeleteHandler&& handle) {
  deleter_ = std::move(handle);
}

ConnManagerIf::DeleteHandler& ConnManagerIf::GetDeleteHandler() {
  return deleter_;
}

void ConnManagerIf::SetConnectHandler(bamboo::net::ConnManagerIf::ConnectHandler&& handle) {
  connector_ = std::move(handle);
}

ConnManagerIf::ConnectHandler& ConnManagerIf::GetConnectHandler() {
  return connector_;
}

bamboo::protocol::ProtocolPtr ConnManagerIf::GetProtocol() {
  return protocol_;
}

}
}