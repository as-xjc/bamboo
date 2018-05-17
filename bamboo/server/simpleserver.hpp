#pragma once

#include <functional>
#include <bamboo/server/serverif.hpp>

namespace bamboo {
namespace server {

/**
 * 简易的服务类
 *
 * @see bamboo::server::ServerIf
 */
class SimpleServer : public ServerIf {
 public:
  using ServerIf::ServerIf;
  virtual ~SimpleServer();

 protected:
  bool PrepareStart() override;
  bool FinishStart() override;
  void StopHandle() override;
  void Configure(boost::program_options::variables_map& map) override;

};

}
}