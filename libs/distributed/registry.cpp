#include "bamboo/distributed/registry.hpp"

#include <array>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <boost/algorithm/string.hpp>

#include <bamboo/log/log.hpp>
#include <bamboo/utility/defer.hpp>
#include <bamboo/define.hpp>
#include <boost/asio/local/connect_pair.hpp>

namespace {
const char* SERVERS_PATH = "/servers";
const char* MASTER_PATH = "/master";
const char* MASTER_NAME = "master";
const char* SERVERS_NAME = "servers";
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

bool Registry::createZkDir(const std::vector<std::string>& dir) {
  if (dir.empty()) return false;

  std::string path;
  char buffer[128]{0};
  int length = 128;

  for (const auto& name : dir) {
    path.append("/").append(name);
    auto ret = zoo_create(zookeeper_.get(), path.c_str(), "", 0, &ZOO_OPEN_ACL_UNSAFE, 0, buffer, length);
    if (ret != ZOK && ret != ZNODEEXISTS) {
      BB_ERROR_LOG("create dir[%s] fail:%s", path.c_str(), zerror(ret));
      return false;
    }
  }

  return true;
}

void Registry::InitServerId() {
  BB_ASSERT(!name_.empty());

  serverId_ = name_;

  boost::uuids::uuid uuid = boost::uuids::random_generator_mt19937()();
  uuid_ = boost::uuids::to_string(uuid);
  serverId_.append(":").append(std::to_string(zone_)).append(":").append(uuid_);

  BB_INFO_LOG("init server id:%s", serverId_.c_str());
}

void Registry::initNodeState() {
  char buffer[128]{0};
  int length = 128;

  if (!createZkDir({MASTER_NAME, name_})) {
    BB_ERROR_LOG("register create master dir fail");
    std::exit(EXIT_FAILURE);
  }

  std::string path = MASTER_PATH;
  path.append("/").append(name_).append("/").append(std::to_string(zone_));
  auto ret = zoo_create(zookeeper_.get(),
                        path.c_str(),
                        serverId_.c_str(),
                        serverId_.size(),
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_EPHEMERAL,
                        buffer,
                        length);

  if (ret != ZOK && ret != ZNODEEXISTS) {
    BB_ERROR_LOG("create master[%s] fail:%s", path.c_str(), zerror(ret));
    std::exit(EXIT_FAILURE);
  }

  ret = zoo_wget(zookeeper_.get(), path.c_str(), &Registry::WatchSelfMasterState, this, buffer, &length, nullptr);
  if (ret == ZOK) {
    if (serverId_ == std::string(buffer, length)) {
      nodeState_ = NodeState::MASTER;
    } else {
      nodeState_ = NodeState::SLAVE;
    }
  } else {
    BB_ERROR_LOG("get master[%s] fail:%s", path.c_str(), zerror(ret));
    nodeState_ = NodeState::SLAVE;
    std::exit(EXIT_FAILURE);
  }

  if (stateChangeHandler_) stateChangeHandler_(nodeState_);
}

void Registry::updateNodeState() {
  const NodeState old = nodeState_;

  DEFER(
      if (old != nodeState_ && stateChangeHandler_) {
        stateChangeHandler_(nodeState_);
      }
      );

  char buffer[128]{0};
  int length = 128;
  std::string path = MASTER_PATH;
  path.append("/").append(name_).append("/").append(std::to_string(zone_));
  auto ret = zoo_create(zookeeper_.get(),
                        path.c_str(),
                        serverId_.c_str(),
                        serverId_.size(),
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_EPHEMERAL,
                        buffer,
                        length);

  if (ret != ZOK && ret != ZNODEEXISTS) {
    BB_ERROR_LOG("create master[%s] fail:%s", path.c_str(), zerror(ret));
    nodeState_ = NodeState::SLAVE;
    return;
  }

  ret = zoo_wget(zookeeper_.get(), path.c_str(), &Registry::WatchSelfMasterState, this, buffer, &length, nullptr);
  if (ret == ZOK) {
    if (serverId_ == std::string(buffer, length)) {
      nodeState_ = NodeState::MASTER;
    } else {
      nodeState_ = NodeState::SLAVE;
    }
  } else {
    BB_ERROR_LOG("get master[%s] fail:%s", path.c_str(), zerror(ret));
    nodeState_ = NodeState::SLAVE;
  }
}

void Registry::WatchSelfMasterState(zhandle_t* zh, int type, int state, const char* path, void* watcherCtx) {
  auto p = reinterpret_cast<Registry*>(watcherCtx);
  p->taskLock_.lock();
  p->taskList_.emplace_back(std::bind(&Registry::updateNodeState, p));
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

void Registry::WatchCreateServer(zhandle_t* zh, int type, int state, const char* path, void* watcherCtx) {
  if (type == ZOO_CREATED_EVENT) {
    auto p = reinterpret_cast<Registry*>(watcherCtx);
    std::string serverpath(path);
    p->taskLock_.lock();
    p->taskList_.emplace_back(std::bind(&Registry::_watchServerZone, p, serverpath));
    p->taskLock_.unlock();
    p->DoWrite();
  }
}

void Registry::_watchServerZone(std::string path) {
  String_vector strv{};
  DEFER(deallocate_String_vector(&strv));

  auto ret = zoo_wget_children(zookeeper_.get(), path.c_str(), &Registry::WatchServerZone, this, &strv);
  if (ret != ZOK) {
    BB_ERROR_LOG("watch server zone[%s] fail:%s", path.c_str(), zerror(ret));
    return;
  }

  std::string _path;
  for (int i = 0; i < strv.count; ++i) {
    _path = path;
    _path.append("/").append(strv.data[i]);
    _watchServerList(_path);
  }
}

void Registry::_watchServerList(std::string path) {
  String_vector strv{};
  DEFER(deallocate_String_vector(&strv));

  auto ret = zoo_wget_children(zookeeper_.get(), path.c_str(), &Registry::WatchServerZoneListChange, this, &strv);
  if (ret != ZOK) {
    BB_ERROR_LOG("watch server zone list[%s] fail:%s", path.c_str(), zerror(ret));
    return;
  }

  std::set<std::string> servers;
  for (int i = 0; i < strv.count; ++i) {
    servers.insert(strv.data[i]);
  }

  std::vector<std::string> paths;
  boost::algorithm::split(paths, path, boost::algorithm::is_any_of("/"));
  std::string server = paths[paths.size()-2];
  int zone = std::stoi(paths[paths.size()-1]);
  updateServerList(server, zone, servers);
}

void Registry::WatchServerZone(zhandle_t* zh, int type, int state, const char* path, void* watcherCtx) {
  if (type == ZOO_CREATED_EVENT) {
    auto p = reinterpret_cast<Registry*>(watcherCtx);
    std::string serverpath(path);
    p->taskLock_.lock();
    p->taskList_.emplace_back(std::bind(&Registry::_watchServerZone, p, serverpath));
    p->taskLock_.unlock();
    p->DoWrite();
  }
}

void Registry::WatchServerZoneListChange(zhandle_t* zh, int type, int state, const char* path, void* watcherCtx) {
  if (type == ZOO_CHILD_EVENT) {
    auto p = reinterpret_cast<Registry*>(watcherCtx);
    std::string serverpath(path);
    p->taskLock_.lock();
    p->taskList_.emplace_back(std::bind(&Registry::_watchServerList, p, serverpath));
    p->taskLock_.unlock();
    p->DoWrite();
  }
}

std::string Registry::getServerInfo(const std::string& type, int zone, const std::string& serverId) {
  std::string path = SERVERS_PATH;
  path.append("/").append(type).append("/").append(std::to_string(zone)).append(serverId);
  char buffer[128]{0};
  int length = 128;
  auto ret = zoo_get(zookeeper_.get(), path.c_str(), 0, buffer, &length, nullptr);
  if (ret == ZOK) {
    return std::string(buffer, length);
  } else {
    return std::string();
  }
}

void Registry::updateServerList(std::string serverType, int zone, const std::set<std::string>& list) {
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
  for (auto& serverId : list) {
    if (!inWatch(serverType, zone, serverId)) continue;

    auto& set = serversList_[serverType];
    if (set.find(serverId) == set.end()) {
      set.insert(serverId);
      if (addServerHandler_) {
        addServerHandler_(serverType, zone, serverId, getServerInfo(serverType, zone, serverId));
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

  std::string path;
  for (const auto& server : watchServer) {
    path = SERVERS_PATH;
    path.append("/").append(server);
    auto ret = zoo_wexists(zookeeper_.get(), path.c_str(), &Registry::WatchCreateServer, this, nullptr);
    if (ret != ZOK) continue;

    _watchServerZone(path);
  }
}

void Registry::Register() {
  char buffer[128]{0};
  std::string message;
  if (serverInfoHandler_) message = serverInfoHandler_();

  const std::string zone = std::to_string(zone_);
  if (!createZkDir({SERVERS_NAME, name_, zone})) {
    BB_ERROR_LOG("register create server dir fail");
    std::exit(EXIT_FAILURE);
  }
  std::string path = SERVERS_PATH;

  path.append("/").append(name_).append("/").append(zone).append("/").append(serverId_);
  auto ret = zoo_create(zookeeper_.get(), path.c_str(), message.c_str(), message.size(), &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, buffer, 128);
  if (ret != ZOK && ret != ZNODEEXISTS) {
    BB_ERROR_LOG("register server[%s] fail:%s", path.c_str(), zerror(ret));
    std::exit(EXIT_FAILURE);
  }

  if (nodeMode_ == NodeMode::MASTER_MASTER) {
    nodeState_ = NodeState::MASTER;
  } else {
    initNodeState();
  }

  if (nodeState_ == NodeState::MASTER) {
    BB_INFO_LOG("init server node state: <master>");
  } else {
    BB_INFO_LOG("init server node state: <slave>");
  }
}

void Registry::Stop() {
  zookeeper_.reset();
}

}
}