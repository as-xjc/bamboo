#pragma once

#include <map>
#include <ctime>
#include <memory>
#include <string>
#include <leveldb/db.h>
#include <boost/utility/string_view.hpp>
#include <bamboo/cache/redis.hpp>

namespace bamboo {
namespace cache {

/**
 * 2级缓冲，db通过leveldb进行保存，后端使用redis作为1级缓冲
 */
class L2Cache final {
 public:
  L2Cache();
  virtual ~L2Cache();

  struct Option {
    bool enableCache; /**< 是否启用本地缓存，关闭则每次读写都是直接通过后端redis进行操作 */
    std::string dbPath; /**< leveldb 的地址，设置了则启用leveldb */
    leveldb::Options dbOption; /**< leveldb 配置数据 */
    leveldb::ReadOptions dbReadOption; /**< leveldb 读选项 */
    leveldb::WriteOptions dbWriteOption; /**< leveldb 写选项 */

    std::string redisIp; /**< redis 的地址，设置了则启用redis */
    uint16_t redisPort; /**< redis 的端口，设置了则启用redis */

    uint32_t defaultExpire; /**< 默认过期秒数 */

    Option(): enableCache(true),
              redisPort(0),
             defaultExpire(10) {}
  };

  /// 初始化
  void Init(const Option& option);

  /**
   * 获取临时数据
   *
   * @note 当本地数据未命中，如果设置了redis，会从redis读取，并且放入cache，数据过期时间默认为10秒
   *
   * @param key 数据的key
   * @return 数据
   */
  std::string Get(boost::string_view key);

  /**
   * 插入临时数据，同步到redis
   * @param key 数据的key
   * @param data 数据
   */
  void Set(boost::string_view key, boost::string_view data);

  /// 删除临时数据
  void Del(boost::string_view key);

  /**
   * 插入持久数据
   *
   * @note 插入到leveldb，属于本地数据，只能通过DbDel删除。数据不会同步到后端redis。
   *
   * @param key
   * @param data
   */
  void DbSet(boost::string_view key, boost::string_view data);

  /// 读取持久数据
  std::string DbGet(boost::string_view key);

  /// 删除持久数据
  void DbDel(boost::string_view key);

  /**
   * 强制读取数据，不优先读取cache的数据，而是直接读取后端redis数据
   * @param key 数据的key
   * @param cache 是否更新cache中的数据
   * @return 读取到的数据
   */
  std::string ForceGet(boost::string_view key, bool cache = true);

  /**
   * 提供给外部的心跳处理函数，让外面控制频率
   * 1. 检查数据过期
   */
  void Heartbeat();

  /**
   * 提供给外部的心跳处理函数，让外面控制频率
   * 保持和redis的keepalive和重连
   */
  void Ping();

 private:
  struct DataInfo {
    std::string data;
    std::time_t createTime;
    uint32_t expire;
    DataInfo():createTime(0), expire(0) {}
  };
  std::map<std::string, std::shared_ptr<DataInfo>> cache_;
  std::unique_ptr<leveldb::DB> db_;
  Option option_;

  Redis redis_;
};

}
}