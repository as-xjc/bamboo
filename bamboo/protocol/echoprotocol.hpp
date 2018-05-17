#pragma once

#include <bamboo/define.hpp>
#include <bamboo/protocol/protocolif.hpp>

namespace bamboo {
namespace protocol {

/**
 * 回响协议
 *
 * @see bamboo::protocol::ProtocolIf
 */
class EchoProtocol : public bamboo::protocol::ProtocolIf {
 public:
  using bamboo::protocol::ProtocolIf::ProtocolIf;
  virtual ~EchoProtocol() {}
  std::size_t ReceiveData(bamboo::net::SocketPtr so, const char* data, std::size_t size) override {
    so->WriteData(data, size);
    return size;
  }
};

}
}
