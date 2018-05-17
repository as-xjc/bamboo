#pragma once

#include <map>
#include <functional>

#include <bamboo/net/acceptorif.hpp>
#include <bamboo/server/serverif.hpp>

namespace bamboo {

namespace aio {
class AioIf;
}

namespace server {

/**
 * 终端控制服务，方便telnet进来对服务进行控制
 */
class Console : public ServerIf {
 public:
  using ServerIf::ServerIf;
  virtual ~Console();

  void Configure(boost::program_options::variables_map& map) override;

  /// 命令处理函数类型
  typedef std::function<std::string(std::vector<std::string>& args)> cmdHandler;

  /**
   * 设置监听端口
   * @param address
   * @param port
   */
  virtual void SetAddress(std::string address, uint16_t port) final;

  /**
   * 注册命令
   * @param cmd 命令，help 和 log 为默认命令，就算注册了也不会生效
   * @param handler 命令处理函数
   * @param help 帮助信息
   */
  virtual void RegCmd(std::string cmd, cmdHandler&& handler, std::string help) final;

 protected:
  bool PrepareStart() override;
  bool FinishStart() override;
  void StopHandle() override;

 private:
  virtual std::size_t readData(bamboo::net::SocketPtr, const char*, std::size_t) final;
  virtual std::string Cmd_Help(std::vector<std::string>& args) final;
  virtual std::string Cmd_Log(std::vector<std::string>& args) final;

  struct CmdInfo {
    std::string cmd;
    std::string help;
    cmdHandler handler;
  };
  std::map<std::string, std::shared_ptr<CmdInfo>> cmds_;

  CmdInfo logCmd_;
  CmdInfo helpCmd_;

  bamboo::net::AcceptorPtr acceptor_;
  std::string address_;
  uint16_t port_{0};
};

}
}

