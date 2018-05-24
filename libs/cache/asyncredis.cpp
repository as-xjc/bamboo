#include "bamboo/cache/asyncredis.hpp"

#include "bamboo/log/log.hpp"

namespace bamboo {
namespace cache {

AsyncRedis::AsyncRedis(boost::asio::io_context& io): socket_(io) {}

AsyncRedis::~AsyncRedis() {
  if (context_ != nullptr) {
    redisAsyncFree(context_);
    context_ = nullptr;
  }
}

void AsyncRedis::SetAddress(const char* address, std::uint16_t port) {
  ip_ = address;
  port_ = port;
}

bool AsyncRedis::Connect() {
  context_ = redisAsyncConnect(ip_.c_str(), port_);
  if (context_->err) {
    BB_ERROR_LOG("async redis connect error:%s", context_->errstr);
    redisAsyncFree(context_);
    context_ = nullptr;
    return false;
  }
  context_->data = reinterpret_cast<void*>(this);

  redisAsyncSetConnectCallback(context_, &AsyncRedis::RedisConnectCallback);
  redisAsyncSetDisconnectCallback(context_, &AsyncRedis::RedisDisconnectCallback);

  context_->ev.data = reinterpret_cast<void*>(this);
  context_->ev.addRead = &AsyncRedis::RedisAddRead;
  context_->ev.delRead = &AsyncRedis::RedisDelRead;
  context_->ev.addWrite = &AsyncRedis::RedisAddWrite;
  context_->ev.delWrite = &AsyncRedis::RedisDelWrite;
  context_->ev.cleanup = &AsyncRedis::RedisCleanup;

  socket_.assign(boost::asio::ip::tcp::v4(), context_->c.fd);
  isRead_ = true;
  isWrite_ = true;
  waitRead();
  waitWrite();
  return true;
}

void AsyncRedis::SetConnectCallback(AsyncRedis::ConnectCallback cb) {
  connectCallback_ = std::move(cb);
}

bool AsyncRedis::IsConnected() {
  return context_ != nullptr;
}

void AsyncRedis::Disconnect() {
  if (context_ != nullptr) {
    redisAsyncDisconnect(context_);
  }
}

void AsyncRedis::SetDisconnectCallback(AsyncRedis::DisconnectCallback cb) {
  disconnectCallback_ = std::move(cb);
}

void AsyncRedis::RedisConnectCallback(const struct redisAsyncContext* c, int status) {
  auto ptr = reinterpret_cast<AsyncRedis*>(c->data);
  if (ptr == nullptr) return;

  if (status == 0) {
    if (ptr->connectCallback_) ptr->connectCallback_(status);
  } else {
    ptr->context_ = nullptr;
  }
}

void AsyncRedis::RedisDisconnectCallback(const struct redisAsyncContext* c, int status) {
  auto ptr = reinterpret_cast<AsyncRedis*>(c->data);
  if (ptr == nullptr) return;

  if (status == 0) {
    if (ptr->disconnectCallback_) ptr->disconnectCallback_(status);
  } else {
    ptr->context_ = nullptr;
  }
}

void AsyncRedis::waitRead() {
  if (!isRead_ || context_ == nullptr) return;

  socket_.async_wait(boost::asio::ip::tcp::socket::wait_read, [this](const boost::system::error_code& ec) {
    if (ec && ec != boost::system::errc::operation_canceled) {
      BB_ERROR_LOG("async redis read error:%s", ec.message().c_str());
      return;
    }

    if (context_ != nullptr) {
      redisAsyncHandleRead(context_);
      waitRead();
    }
  });
}

void AsyncRedis::waitWrite() {
  if (!isWrite_ || context_ == nullptr) return;

  socket_.async_wait(boost::asio::ip::tcp::socket::wait_write, [this](const boost::system::error_code& ec) {
    if (ec && ec != boost::system::errc::operation_canceled) {
      BB_ERROR_LOG("async redis write error:%s", ec.message().c_str());
      return;
    }

    if (context_ != nullptr) {
      redisAsyncHandleWrite(context_);
      waitWrite();
    }
  });
}

void AsyncRedis::RedisAddRead(void* privdata) {
  auto ptr = reinterpret_cast<AsyncRedis*>(privdata);
  if (ptr == nullptr || ptr->context_ == nullptr) return;

  ptr->isRead_ = true;
  ptr->waitRead();
}

void AsyncRedis::RedisDelRead(void* privdata) {
  auto ptr = reinterpret_cast<AsyncRedis*>(privdata);
  if (ptr == nullptr || ptr->context_ == nullptr) return;

  ptr->isRead_ = false;
}

void AsyncRedis::RedisAddWrite(void* privdata) {
  auto ptr = reinterpret_cast<AsyncRedis*>(privdata);
  if (ptr == nullptr || ptr->context_ == nullptr) return;

  ptr->isWrite_ = true;
  ptr->waitWrite();
}

void AsyncRedis::RedisDelWrite(void* privdata) {
  auto ptr = reinterpret_cast<AsyncRedis*>(privdata);
  if (ptr == nullptr || ptr->context_ == nullptr) return;

  ptr->isWrite_ = false;
}

void AsyncRedis::RedisCleanup(void* privdata) {
  auto ptr = reinterpret_cast<AsyncRedis*>(privdata);
  if (ptr == nullptr || ptr->context_ == nullptr) return;
  ptr->socket_.cancel();
  ptr->socket_.release();
  ptr->context_ = nullptr;
  ptr->id_ = 0;
  ptr->commands_.clear();
}

uint32_t AsyncRedis::genId() {
  ++id_;
  if (id_ == 0) ++id_;

  return id_;
}

void AsyncRedis::Command(AsyncRedis::CommandCallback cb, const char* format, ...) {
  if (context_ == nullptr) return;

  uint32_t id = genId();
  commands_[id] = std::move(cb);

  void* data = reinterpret_cast<void*>(id);
  va_list ap;
  va_start(ap, format);
  redisvAsyncCommand(context_, &AsyncRedis::CommandHandle, data, format, ap);
  va_end(ap);
}

void AsyncRedis::Command(const char* format, ...) {
  if (context_ == nullptr) return;

  va_list ap;
  va_start(ap, format);
  redisvAsyncCommand(context_, &AsyncRedis::CommandHandle, nullptr, format, ap);
  va_end(ap);
}

void AsyncRedis::CommandHandle(struct redisAsyncContext* e, void* r, void* data) {
  if (e == nullptr || r == nullptr || data == nullptr) return;

  auto ptr = reinterpret_cast<AsyncRedis*>(e->data);
  auto id = static_cast<uint32_t>(reinterpret_cast<std::ptrdiff_t>(data));

  auto it = ptr->commands_.find(id);
  if (it != ptr->commands_.end()) {
    if (it->second) {
      auto reply = reinterpret_cast<redisReply*>(r);
      it->second(reply);
    }
    ptr->commands_.erase(it);
  }
}

}
}