#include "bamboo/distributed/registry.hpp"

#include <array>
#include <bamboo/log/log.hpp>
#include <bamboo/utility/defer.hpp>
#include <bamboo/define.hpp>
#include <boost/asio/local/connect_pair.hpp>

namespace {
const char* SERVERS_PATH = "/servers";
const char* RUNNING_PATH = "/running";
const char* MASTER_NAME = "master";
}

namespace bamboo {
namespace distributed {

Registry::Registry(boost::asio::io_context& io) : sender_(io), receiver_(io) {
  boost::asio::local::connect_pair(sender_, receiver_);
  DoRead();
}
Registry::~Registry() {}

void Registry::Init(std::string address) {
  address_ = address;
}

void Registry::Connect() {
  zhandle_t* p = zookeeper_init(address_.c_str(), nullptr, 5000, nullptr, this, 0);
  if (p == nullptr) {
    BB_ERROR_LOG("zookeeper init fail");
    std::exit(EXIT_FAILURE);
  }

  std::unique_ptr<zhandle_t, decltype(&zookeeper_close)> zk(p, &zookeeper_close);
  zookeeper_ = std::move(zk);
}

void Registry::SetServerType(std::string name, int zone, bamboo::distributed::NodeMode mode) {
  name_ = name;
  zone_ = zone;
  nodeMode_ = mode;
}

void Registry::InitServerId() {
  BB_ASSERT(!name_.empty());

  std::array<char, 128> buffer{};

  std::string path = SERVERS_PATH;
  std::string message = "server list";
  auto ret = zoo_create(zookeeper_.get(),
                        path.c_str(),
                        message.c_str(),
                        message.size(),
                        &ZOO_OPEN_ACL_UNSAFE,
                        0,
                        buffer.data(),
                        buffer.size());
  if (ret != ZOK && ret != ZNODEEXISTS) {
    BB_ERROR_LOG("create path[%s] fail:%s", path.c_str(), zerror(ret));
    std::exit(EXIT_FAILURE);
  }

  // /servers/name
  path.append("/").append(name_);
  ret = zoo_create(zookeeper_.get(),
                   path.c_str(),
                   message.c_str(),
                   message.size(),
                   &ZOO_OPEN_ACL_UNSAFE,
                   0,
                   buffer.data(),
                   buffer.size());
  if (ret != ZOK && ret != ZNODEEXISTS) {
    BB_ERROR_LOG("create path[%s] fail:%s", path.c_str(), zerror(ret));
    std::exit(EXIT_FAILURE);
  }

  // /servers/name/zone
  path.append("/").append(std::to_string(zone_));
  ret = zoo_create(zookeeper_.get(), path.c_str(), "zone", 4, &ZOO_OPEN_ACL_UNSAFE, 0, buffer.data(), buffer.size());
  if (ret != ZOK && ret != ZNODEEXISTS) {
    BB_ERROR_LOG("create path[%s] fail:%s", path.c_str(), zerror(ret));
    std::exit(EXIT_FAILURE);
  }

  // /servers/name/zone/name:zone:xxxxxx
  path.append("/").append(name_).append(":").append(std::to_string(zone_)).append(":");
  ret = zoo_create(zookeeper_.get(), path.c_str(), "name", 4, &ZOO_OPEN_ACL_UNSAFE,
                   ZOO_EPHEMERAL | ZOO_SEQUENCE, buffer.data(), buffer.size());
  if (ret != ZOK) {
    BB_ERROR_LOG("create path[%s] fail:%s", path.c_str(), zerror(ret));
    std::exit(EXIT_FAILURE);
  }

  std::string fullPath(buffer.data());
  BB_DEBUG_LOG("create path:%s", fullPath.c_str());
  auto pos = fullPath.rfind('/');
  if (pos == std::string::npos) {
    BB_ERROR_LOG("create server id fail:%s", fullPath.c_str());
    std::exit(EXIT_FAILURE);
  }
  serverId_ = fullPath.substr(pos + 1, fullPath.size() - pos);
  BB_INFO_LOG("init server id:%s", serverId_.c_str());

  if (nodeMode_ == NodeMode::MASTER_MASTER) {
    nodeState_ = NodeState::MASTER;
  } else {
    initNodeState(false);
  }

  if (nodeState_ == NodeState::MASTER) {
    BB_INFO_LOG("init server node state: <master>");
  } else {
    BB_INFO_LOG("init server node state: <slave>");
  }
}

void Registry::initNodeState(bool notify) {
  const NodeState old = nodeState_;

  char buffer[128]{0};
  int length = 128;
  std::string master = SERVERS_PATH;
  master.append("/").append(name_).append("/").append(std::to_string(zone_)).append("/").append(MASTER_NAME);
  auto ret = zoo_create(zookeeper_.get(), master.c_str(), serverId_.c_str(),
                        serverId_.size(), &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, buffer, length);
  if (ret != ZOK && ret != ZNODEEXISTS) {
    BB_ERROR_LOG("create master[%s] fail:%s", master.c_str(), zerror(ret));
    if (!notify) std::exit(EXIT_FAILURE);
  }

  ret = zoo_wget(zookeeper_.get(), master.c_str(), &Registry::watchMaster, this, buffer, &length, nullptr);
  if (ret == ZOK) {
    if (serverId_ == std::string(buffer, length)) {
      nodeState_ = NodeState::MASTER;
    } else {
      nodeState_ = NodeState::SLAVE;
    }
  } else {
    BB_ERROR_LOG("get master[%s] fail:%s", master.c_str(), zerror(ret));
    nodeState_ = NodeState::SLAVE;
    if (!notify) std::exit(EXIT_FAILURE);
  }

  if (notify && old != nodeState_ && stateChangeHandler_) {
    stateChangeHandler_(nodeState_);
  }
}

void Registry::watchMaster(zhandle_t* zh, int type, int state, const char* path, void* watcherCtx) {
  auto p = reinterpret_cast<Registry*>(watcherCtx);
  p->taskLock_.lock();
  p->taskList_.emplace_back(std::bind(&Registry::initNodeState, p, true));
  p->taskLock_.unlock();
  p->DoWrite();
}

std::string Registry::GetServerId() {
  return serverId_;
}

NodeState Registry::GetState() {
  return nodeState_;
}

void Registry::DoRead() {
  receiver_.async_read_some(boost::asio::buffer(buffer_.data(), buffer_.size()),
                            [this](const boost::system::error_code& ec, std::size_t rd) {
                              taskLock_.lock();
                              for (auto& f : taskList_) {
                                f();
                              }
                              taskList_.clear();
                              taskLock_.unlock();
                              DoRead();
                            });
}

void Registry::DoWrite() {
  static const char c = 'i';
  sender_.write_some(boost::asio::buffer(&c, 1));
}

void Registry::SetNodeStateChangeHandler(bamboo::distributed::Registry::NodeStateHandler&& handler) {
  stateChangeHandler_ = std::move(handler);
}

void Registry::SetAddServerHandler(bamboo::distributed::Registry::AddServerHandler&& handler) {
  addServerHandler_ = std::move(handler);
}

void Registry::SetDelServerHandler(bamboo::distributed::Registry::DelServerHandler&& handler) {
  delServerHandler_ = std::move(handler);
}

void Registry::SetServerInfoHandler(bamboo::distributed::Registry::ServerInfoHandler&& handler) {
  serverInfoHandler_ = std::move(handler);
}

void Registry::AddWatchServer(std::string type, int zone) {
  watchServerZone_[type].insert(zone);
}

void Registry::AddWatchServer(std::string type) {
  watchServers_.insert(type);
}

void Registry::watchServer(zhandle_t* zh, int type, int state, const char* path, void* watcherCtx) {
  if (type == ZOO_CREATED_EVENT) {
    auto p = reinterpret_cast<Registry*>(watcherCtx);
    std::string serverpath(path);
    p->taskLock_.lock();
    p->taskList_.emplace_back(std::bind(&Registry::_watchServerList, p, serverpath));
    p->taskLock_.unlock();
    p->DoWrite();
  }
}

void Registry::_watchServerList(std::string path) {
  String_vector strv{};
  DEFER(deallocate_String_vector(&strv));

  auto ret = zoo_wget_children(zookeeper_.get(), path.c_str(), &Registry::watchServerList, this, &strv);
  if (ret != ZOK) {
    BB_ERROR_LOG("watch serverList[%s] fail:%s", path.c_str(), zerror(ret));
    return;
  }

  std::set<std::string> servers;
  for (int i = 0; i < strv.count; ++i) {
    servers.insert(strv.data[i]);
  }

  auto it = path.rfind('/');
  std::string server = path.substr(it+1, path.size()-it);
  checkServerList(server, std::move(servers));
}

void Registry::watchServerList(zhandle_t* zh, int type, int state, const char* path, void* watcherCtx) {
  if (type == ZOO_CHILD_EVENT) {
    auto p = reinterpret_cast<Registry*>(watcherCtx);
    std::string serverpath(path);
    p->taskLock_.lock();
    p->taskList_.emplace_back(std::bind(&Registry::_watchServerList, p, serverpath));
    p->taskLock_.unlock();
    p->DoWrite();
  }
}

std::tuple<std::string, int> Registry::parserServerId(const std::string& serverId) {
  auto first = serverId.find(':');
  auto end = serverId.rfind(':');
  if (first == std::string::npos || end == std::string::npos || first == end) {
    return std::make_tuple("", 0);
  }

  std::string type = serverId.substr(0, first);
  std::string zonestr = serverId.substr(first+1, end-first-1);
  int zone = std::stoi(zonestr);
  return std::make_tuple(type, zone);
}

std::string Registry::getServerInfo(const std::string& type, const std::string& serverId) {
  std::string path = RUNNING_PATH;
  path.append("/").append(type).append("/").append(serverId);
  char buffer[128]{0};
  int length = 128;
  auto ret = zoo_get(zookeeper_.get(), path.c_str(), 0, buffer, &length, nullptr);
  if (ret == ZOK) {
    return std::string(buffer, length);
  } else {
    return std::string();
  }
}

void Registry::checkServerList(std::string serverType, std::set<std::string> list) {
  // check del
  {
    auto it = serversList_.find(serverType);
    if (it != serversList_.end()) {
      for (auto it2 = it->second.begin(); it2 != it->second.end(); ) {
        if (list.find(*it2) == list.end()) {
          if (delServerHandler_) delServerHandler_(*it2);
          it2 = it->second.erase(it2);
        } else {
          ++it2;
        }
      }
    }
  }

  // check add
  std::string server;
  int zone;
  for (auto& serverId : list) {
    std::tie(server, zone) = parserServerId(serverId);
    if (!inWatch(server, zone, serverId)) continue;

    auto& set = serversList_[server];
    if (set.find(serverId) == set.end()) {
      set.insert(serverId);
      if (addServerHandler_) {
        addServerHandler_(server, zone, serverId, getServerInfo(server, serverId));
      }
    }
  }
}

bool Registry::inWatch(std::string server, int zone, std::string serverId) {
  if (watchServers_.find(server) != watchServers_.end()) return true;

  auto it = watchServerZone_.find(server);
  if (it == watchServerZone_.end()) return false;

  return it->second.find(zone) != it->second.end();
}

void Registry::StartWatch() {
  std::set<std::string> watchServer = watchServers_;
  for (auto& it : watchServerZone_) {
    watchServer.insert(it.first);
  }

  char buffer[128]{0};
  auto ret = zoo_create(zookeeper_.get(), RUNNING_PATH, "running", 7, &ZOO_OPEN_ACL_UNSAFE, 0, buffer, 128);
  if (ret != ZOK && ret != ZNODEEXISTS) {
    BB_ERROR_LOG("create running[%s] fail:%s", RUNNING_PATH, zerror(ret));
    std::exit(EXIT_FAILURE);
  }

  for (auto& server : watchServer) {
    std::string path = RUNNING_PATH;
    path.append("/").append(server);
    auto ret = zoo_wexists(zookeeper_.get(), path.c_str(), &Registry::watchServer, this, nullptr);
    if (ret != ZOK) continue;

    _watchServerList(path);
  }
}

void Registry::Register() {
  char buffer[128]{0};
  std::string message;
  if (serverInfoHandler_) message = serverInfoHandler_();

  std::string path = RUNNING_PATH;
  path.append("/").append(name_);
  auto ret = zoo_create(zookeeper_.get(), path.c_str(), "list", 4, &ZOO_OPEN_ACL_UNSAFE, 0, buffer, 128);
  if (ret != ZOK && ret != ZNODEEXISTS) {
    BB_ERROR_LOG("register running[%s] fail:%s", path.c_str(), zerror(ret));
    std::exit(EXIT_FAILURE);
  }

  path.append("/").append(serverId_);
  ret = zoo_create(zookeeper_.get(), path.c_str(), message.c_str(), message.size(), &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, buffer, 128);
  if (ret != ZOK && ret != ZNODEEXISTS) {
    BB_ERROR_LOG("register running[%s] fail:%s", path.c_str(), zerror(ret));
    std::exit(EXIT_FAILURE);
  }
}

void Registry::Stop() {
  zookeeper_.reset();
}

}
}