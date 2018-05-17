#include "bamboo/cache/l2cache.hpp"
#include "bamboo/log/log.hpp"

namespace bamboo {
namespace cache {

L2Cache::L2Cache() {}

L2Cache::~L2Cache() {}

std::string L2Cache::Get(boost::string_view key) {
  if (option_.enableCache) {
    auto it = cache_.find(key.data());
    if (it != cache_.end()) {
      auto now = std::time(nullptr);
      if (now - it->second->createTime < it->second->expire) {
        return it->second->data;
      } else {
        cache_.erase(it);
      }
    }
  }

  return ForceGet(key, true);
}

void L2Cache::Set(boost::string_view key, boost::string_view data) {
  if (option_.enableCache) {
    auto it = cache_.find(key.data());
    if (it != cache_.end()) {
      it->second->createTime = std::time(nullptr);
      it->second->data = data.to_string();
    } else {
      auto ptr = std::make_shared<DataInfo>();
      ptr->data = data.to_string();
      ptr->expire = option_.defaultExpire;
      ptr->createTime = std::time(nullptr);
      cache_.insert(std::make_pair(key.data(), ptr));
    }
  }

  if (redis_) {
    redis_.Command("SET %b %b", key.data(), key.size(), data.data(), data.size());
  }
}

void L2Cache::Del(boost::string_view key) {
  if (option_.enableCache) {
    cache_.erase(key.to_string());
  }

  if (redis_) {
    redis_.Command("DEL %s", key.data());
  }
}

void L2Cache::DbSet(boost::string_view key, boost::string_view data) {
  if (db_) {
    auto ok = db_->Put(option_.dbWriteOption, leveldb::Slice(key.data(), key.size()),
                       leveldb::Slice(data.data(), data.size()));
    if (ok.ok()) {
      BB_DEBUG_LOG("set %s - %s", key.data(), data.data());
      return;
    }

    BB_ERROR_LOG("set error %s - %s", key.data(), ok.ToString().c_str());
  }
}

std::string L2Cache::DbGet(boost::string_view key) {
  if (db_) {
    leveldb::Slice k(key.data(), key.length());
    std::string data;
    db_->Get(option_.dbReadOption, k, &data);
  }

  return "";
}

void L2Cache::DbDel(boost::string_view key) {
  if (db_) {
    leveldb::Slice data(key.data(), key.length());
    db_->Delete(option_.dbWriteOption, data);
  }
}

std::string L2Cache::ForceGet(boost::string_view key, bool cache) {
  if (!redis_) return "";

  auto reply = redis_.Command("GET %s", key.data());
  if (reply == nullptr || reply->type != REDIS_REPLY_STRING || reply->len < 1) {
    return "";
  }

  if (option_.enableCache && cache) {
    auto it = cache_.find(key.data());
    if (it != cache_.end()) {
      it->second->createTime = std::time(nullptr);
      it->second->data = std::string(reply->str, reply->len);
    } else {
      auto ptr = std::make_shared<DataInfo>();
      ptr->data = std::string(reply->str, reply->len);
      ptr->expire = option_.defaultExpire;
      ptr->createTime = std::time(nullptr);
      cache_.insert(std::make_pair(key.data(), ptr));
    }
  }
  return std::string(reply->str, reply->len);
}

void L2Cache::Heartbeat() {
  auto now = std::time(nullptr);
  for (auto it = cache_.begin(); it != cache_.end(); ) {
    if (now - it->second->createTime > it->second->expire) {
      it = cache_.erase(it);
    } else {
      ++it;
    }
  }
}

void L2Cache::Ping() {
  if (!redis_) return;

  redis_.Ping();
}

void L2Cache::Init(const bamboo::cache::L2Cache::Option& option) {
  option_ = option;
  if (!option_.dbPath.empty()) {
    leveldb::DB* db;
    leveldb::Status status = leveldb::DB::Open(option_.dbOption, option_.dbPath, &db);
    if (status.ok()) {
      db_.reset(db);
    } else {
      BB_ERROR_LOG("leveldb error:%s", status.ToString().c_str());
      std::exit(EXIT_FAILURE);
    }
  }

  if (option_.redisPort > 0 && !option_.redisIp.empty()) {
    if (!redis_.Connect(option_.redisIp.c_str(), option_.redisPort)) {
      std::exit(EXIT_FAILURE);
    }
  }
}

}
}