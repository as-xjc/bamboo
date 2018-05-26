#include <bamboo/server/console.hpp>
#include <bamboo/net/simpleacceptor.hpp>
#include <bamboo/net/simpleconnmanager.hpp>

#include <boost/algorithm/string.hpp>
#include <iostream>

#include <bamboo/log/log.hpp>

namespace bamboo {
namespace server {

Console::~Console() {}

void Console::Configure(boost::program_options::variables_map& map) {}

bool Console::PrepareStart() {
  auto acceptor = CreateAcceptor<bamboo::net::SimpleAcceptor>(address_, port_);
  auto mgr = acceptor->CreateConnManager<bamboo::net::SimpleConnManager>();
  mgr->SetReadHandler(std::bind(&Console::readData, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  return true;
}

std::size_t Console::readData(bamboo::net::SocketPtr socket, const char* data, std::size_t size) {
  std::string msg(data, size);
  boost::trim_if(msg, boost::is_any_of("\n"));

  std::vector<std::string> strs;
  boost::split(strs, msg, boost::is_any_of(" "));
  if (strs.empty()) return size;

  std::string cmd = strs.front();
  strs.erase(strs.begin());
  std::string result;
  if (cmd == "exit") {
    socket->Close();
    return size;
  } else if (cmd == helpCmd_.cmd) {
    if (helpCmd_.handler) result = helpCmd_.handler(strs);
  } else if (cmd == logCmd_.cmd) {
    if (logCmd_.handler) result = logCmd_.handler(strs);
  } else {
    auto it = cmds_.find(cmd);
    if (it != cmds_.end() && it->second->handler) {
      try {
        result = it->second->handler(strs);
      } catch(std::exception& e) {
        result = "exception:";
        result.append(e.what());
      }
    }
  }
  if (!result.empty()) {
    result.append("\n");
    socket->WriteData(result.data(), result.length());
  }
  return size;
}

bool Console::FinishStart() {
  helpCmd_.cmd = "help";
  helpCmd_.handler = std::bind(&Console::Cmd_Help, this, std::placeholders::_1);
  helpCmd_.help = "print all cmd help info";

  logCmd_.cmd = "log";
  logCmd_.handler = std::bind(&Console::Cmd_Log, this, std::placeholders::_1);
  logCmd_.help = "if args null, get level. set [debug,info,warn,error] is set log level";
  return true;
}

void Console::StopHandle() {}

void Console::SetAddress(std::string address, uint16_t port) {
  address_ = address;
  port_ = port;
}

void Console::RegCmd(std::string cmd,
                     cmdHandler&& handler,
                     std::string help) {
  if (cmd.empty() || !handler) return;

  auto ptr = std::make_shared<CmdInfo>();
  ptr->cmd = cmd;
  ptr->handler = std::move(handler);
  ptr->help = help;

  cmds_.insert(std::make_pair(cmd, ptr));
}

std::string Console::Cmd_Help(std::vector<std::string>& args) {
  std::stringstream os;
  os << "help\t\t" << helpCmd_.help << "\n";
  os << "log\t\t" << logCmd_.help << "\n";
  os << "exit\t\tquit\n";
  for (auto& it : cmds_) {
    os << it.second->cmd << "\t\t" << it.second->help << "\n";
  }
  return os.str();
}

std::string Console::Cmd_Log(std::vector<std::string>& args) {
  std::stringstream os;
  os << "current level:" << bamboo::log::Level2Name(bamboo::log::GetLevel()) << "\n";
  if (args.empty()) return os.str();

  std::string level = args.front();
  level = boost::to_lower_copy<std::string>(level);
  if (level == "debug") {
    bamboo::log::SetLevel(LOG_DEBUG);
  } else if (level == "info") {
    bamboo::log::SetLevel(LOG_INFO);
  } else if (level == "warn") {
    bamboo::log::SetLevel(LOG_WARNING);
  } else {
    bamboo::log::SetLevel(LOG_ERR);
  }

  os << "new level:" << bamboo::log::Level2Name(bamboo::log::GetLevel()) << "\n";
  return os.str();
}
}
}