#pragma once

#include <bamboo/net/connectorif.hpp>

namespace bamboo {
namespace net {

/**
 * 简易的连接助手类
 *
 * @see bamboo::net::ConnectorIf
 */
class SimpleConnector : public ConnectorIf {
 public:
  using ConnectorIf::ConnectorIf;
  virtual ~SimpleConnector() {}
};

}
}
