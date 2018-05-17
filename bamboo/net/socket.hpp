#pragma once

#include <list>
#include <bamboo/define.hpp>
#include <bamboo/net/socketif.hpp>
#include <bamboo/buffer/fixedbuffer.hpp>

namespace bamboo {
namespace net {

/**
 * socket类
 *
 * @see bamboo::net::SocketIf
 */
class Socket : public SocketIf,
               public std::enable_shared_from_this<Socket> {
 public:
  using SocketIf::SocketIf;
  virtual ~Socket();

  void ReadData() override;
  void WriteData(const char* data, std::size_t size) override;
  void WriteData(bamboo::protocol::MessageIf* message) override;
  void WriteData(std::unique_ptr<std::string>&& buffer) override;
  void Close() override;

 private:
  void DoWriteData();

  bamboo::buffer::FixedBuffer<1024*128> buffer_;
  std::list<std::shared_ptr<std::string>> list_;
};

}
}

