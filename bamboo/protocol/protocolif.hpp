#pragma once

#include <functional>
#include <bamboo/define.hpp>
#include <bamboo/net/socket.hpp>

namespace bamboo {
namespace protocol {

/**
 * 协议处理基类
 */
class ProtocolIf {
 public:
  /// 默认的构造函数
  ProtocolIf();

  /// 默认的析构函数
  virtual ~ProtocolIf();

  /**
   * 协议处理数据总入口
   * @param socket 数据来源socket
   * @param data 数据
   * @param size 长度
   * @return 消耗的长度
   */
  virtual std::size_t ReceiveData(bamboo::net::SocketPtr socket, const char* data, std::size_t size) = 0;

};

using ProtocolPtr = std::shared_ptr<ProtocolIf>;

}
}