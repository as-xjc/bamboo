#pragma once

#include <memory>
#include <string>
#include <list>
#include <map>
#include <set>
#include <thread>

#include <boost/asio/io_context.hpp>
#include <boost/asio/local/stream_protocol.hpp>

#include <zookeeper/zookeeper.h>

namespace bamboo {

namespace aio {
class AioIf;
}

namespace distributed {

/**
 * 节点模式
 */
enum class NodeMode {
  MASTER_SLAVE, /**< 主从模式，一个区域内只有一个主 */
  MASTER_MASTER /**< 主主模式，一个区域内所有的服务都是主*/
};

/**
 * 节点的当前状态
 */
enum class NodeState {
  INIT, /**< 初始化中 */
  MASTER, /**< 主状态 */
  SLAVE /**< 从状态 */
};

/**
 * 服务发现类,用来处理服务注册和发现。这是一个可选服务。
 */
class Registry {
 public:
  /// 默认的析构函数
  virtual ~Registry();

  /**
   * 初始化服务发现的地址
   * @param address zookeeper的地址
   */
  virtual void Init(std::string address) final;

  /**
   * 设置本服务的属性
   *
   * @brief 每类服务可以拥有多个区域，每个区域可以设置主从或主主模式。
   *        服务可以通过主从模式，部署多个区域，实现每个区域的主备切换，达到高可用
   *
   * @param type 服务类型
   * @param zone 服务所在的区域
   * @param mode 节点类型
   */
  virtual void SetServerType(std::string type, int zone, NodeMode mode) final;

  /**
   * 注册监听某个服务类型下的某个区域的服务
   * @param type 服务类型
   * @param zone 服务所在的区域
   */
  virtual void AddWatchServer(std::string type, int zone) final;

  /**
   * 注册监听某种服务类型的所有区域下的服务
   * @param type 服务类型
   */
  virtual void AddWatchServer(std::string type) final;

  /// 获取当前的节点状态
  virtual NodeState GetState() final;

  /// 获取服务注册生成的全局唯一的服务id
  virtual std::string GetServerId() final;

  /// 节点状态改变回调函数类型
  using NodeStateHandler = std::function<void(NodeState)>;

  /**
   * 注册节点主从状态切换的回调处理函数
   * 回调函数类型：(NodeState 新的节点状态)
   * @param handler 回调函数
   */
  virtual void SetNodeStateChangeHandler(NodeStateHandler&& handler) final;

  /// 服务删除回调处理函数类型
  using DelServerHandler = std::function<void(std::string)>;

  /// 注册服务移除的回调处理函数
  virtual void SetDelServerHandler(DelServerHandler&& handler) final;

  /// 服务添加回调处理函数类型
  using AddServerHandler = std::function<void(std::string, int, std::string, std::string)>;

  /**
   * 注册服务发现的回调处理函数
   * 回调函数类型：(string serverType, int zone, string serverId, string serverInfo)
   * @param handler 回调函数
   */
  virtual void SetAddServerHandler(AddServerHandler&& handler) final;

  /// 服务信息获取函数类型
  using ServerInfoHandler =std::function<std::string()>;

  /**
   * 设置获取当前服务信息的处理函数。用于为其他服务提供该服务的信息
   * @param handler 调用函数
   */
  virtual void SetServerInfoHandler(ServerInfoHandler&& handler) final;

  /// 关闭服务
  virtual void Stop() final;

 protected:
  /// 默认的构造函数，不允许直接创建，只能通过AioIf类创建
  Registry(boost::asio::io_context& io);

  /// 连接到zookeeper
  virtual void Connect() final;

  /// 开始监听注册的服务
  virtual void StartWatch() final;

  /// 注册自己到服务中心
  virtual void Register() final;

  /**
   * 初始化服务id，为该服务获得一个全局唯一的id
   * 服务id格式：type:zone:uuid
   */
  virtual void InitServerId() final;

  /// 便于 Aio 创建类对象
  friend class bamboo::aio::AioIf;

 private:
  /// 创建zk的路径
  bool createZkDir(const std::vector<std::string>& dir);

  /// 初始化服务节点的服节点状态，如果服务是 MASTER_SLAVE 模式，同时会监听 master 的变化，一旦有变动则处理主从状态回调
  void initNodeState();

  /// 更新服务节点的节点状态
  void updateNodeState();

  /**
   * 处理某个服务类型的服务列表变动
   * @param serverType 服务类型
   * @param zone 服务区域
   * @param list 当前的服务列表
   */
  void updateServerList(std::string serverType, int zone, const std::set<std::string>& list);

  /**
   * 判断服务是否被设置为需要监听
   * @param server 服务类型
   * @param zone 区域
   * @param serverId 服务id
   * @return 是否需要监听
   */
  bool inWatch(std::string server, int zone, std::string serverId);

  /**
   * 获取服务的服务信息
   * 该信息是保存在 /servers/type/zone/serverId 中
   * @param type 服务类型
   * @param zone 服务区域
   * @param serverId 服务id
   * @return 服务信息
   */
  std::string getServerInfo(const std::string& type, int zone, const std::string& serverId);

  /// 监听自身master变化的处理函数, 在`MASTER_SLAVE`模式下才会有效
  static void WatchSelfMasterState(zhandle_t* zh, int type, int state, const char* path, void* watcherCtx);

  /// 监听新增服务的处理函数
  static void WatchCreateServer(zhandle_t* zh, int type, int state, const char* path, void* watcherCtx);

  /// 监听服务新增区域的处理函数
  static void WatchServerZone(zhandle_t* zh, int type, int state, const char* path, void* watcherCtx);

  /// 新增服务区域处理
  void _watchServerZone(std::string path);

  /// 监听服务区域下，服务列表变化的处理函数
  static void WatchServerZoneListChange(zhandle_t* zh, int type, int state, const char* path, void* watcherCtx);

  /// 服务区域下的服务列表变化处理
  void _watchServerList(std::string path);

  /// 发送信号
  void DoRead();

  /// 用于触发主线程的回调
  void DoWrite();

  std::unique_ptr<zhandle_t, decltype(&zookeeper_close)> zookeeper_{nullptr, nullptr};

  std::string address_;
  std::string serverId_;
  std::string uuid_;
  std::string name_;
  int zone_{0};
  NodeMode nodeMode_{NodeMode::MASTER_SLAVE};
  NodeState nodeState_{NodeState::INIT};
  NodeStateHandler stateChangeHandler_;
  DelServerHandler delServerHandler_;
  AddServerHandler addServerHandler_;
  ServerInfoHandler serverInfoHandler_;

  /// 需要监听的服务类型下的区域, <server : set<zone>>
  std::map<std::string, std::set<int>> watchServerZone_;

  /// 需要监听的服务类型
  std::set<std::string> watchServers_;

  /// 当前保存的服务列表,server : set<serverId>
  std::map<std::string, std::set<std::string>> serversList_;

  std::array<char, 4> buffer_;
  boost::asio::local::stream_protocol::socket sender_;
  boost::asio::local::stream_protocol::socket receiver_;
  std::list<std::function<void()>> taskList_;
  std::mutex taskLock_;
};

}
}

