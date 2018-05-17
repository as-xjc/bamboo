#pragma once

#include <functional>

#include <bamboo/net/acceptorif.hpp>

namespace bamboo {
namespace net {

/**
 * 简易的监听助手类
 *
 * @see bamboo::net::AcceptorIf
 */
class SimpleAcceptor : public AcceptorIf, public std::enable_shared_from_this<SimpleAcceptor> {
 public:
  using AcceptorIf::AcceptorIf;
  virtual ~SimpleAcceptor();

  void Start() override;
  void Stop() override;

 private:
  void DoAccept();
};

}
}

