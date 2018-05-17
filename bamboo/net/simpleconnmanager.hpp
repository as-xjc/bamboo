#pragma once

#include <bamboo/define.hpp>

#include <bamboo/net/connmanagerif.hpp>
#include <bamboo/net/socket.hpp>

namespace bamboo {
namespace net {

/**
 * 简易的链接管理类
 *
 * @see bamboo::net::ConnManagerIf
 */
class SimpleConnManager : public ConnManagerIf,
                          public std::enable_shared_from_this<SimpleConnManager> {
 public:
  using ConnManagerIf::ConnManagerIf;
  virtual ~SimpleConnManager();

  SocketPtr OnConnect(boost::asio::ip::tcp::socket&& socket) override;
  void OnDelete(uint32_t socket) override;
  void WriteData(uint32_t socket, const char* data, std::size_t size) override;

 private:
  std::map<uint32_t, SocketPtr> sockets_;
};

}
}
