#include "bamboo/cache/asyncredisif.hpp"
#include "bamboo/log/log.hpp"

namespace bamboo {
namespace cache {

AsyncRedisIf::AsyncRedisIf(boost::asio::io_context& io): socket_(io) {}

AsyncRedisIf::~AsyncRedisIf() {
  if (context_ != nullptr) {
    redisAsyncFree(context_);
    context_ = nullptr;
  }
}

void AsyncRedisIf::SetAddress(const char* address, std::uint16_t port) {
  ip_ = address;
  port_ = port;
}

bool AsyncRedisIf::Connect() {
  context_ = redisAsyncConnect(ip_.c_str(), port_);
  if (context_ == nullptr) {
    BB_ERROR_LOG("async redis connect error");
    return false;
  }

  if (context_->err) {
    BB_ERROR_LOG("async redis connect error:%s", context_->errstr);
    redisAsyncFree(context_);
    context_ = nullptr;
    return false;
  }
  context_->data = reinterpret_cast<void*>(this);

  redisAsyncSetConnectCallback(context_, &AsyncRedisIf::RedisConnectCallback);
  redisAsyncSetDisconnectCallback(context_, &AsyncRedisIf::RedisDisconnectCallback);

  context_->ev.data = reinterpret_cast<void*>(this);
  context_->ev.addRead = &AsyncRedisIf::RedisAddRead;
  context_->ev.delRead = &AsyncRedisIf::RedisDelRead;
  context_->ev.addWrite = &AsyncRedisIf::RedisAddWrite;
  context_->ev.delWrite = &AsyncRedisIf::RedisDelWrite;
  context_->ev.cleanup = &AsyncRedisIf::RedisCleanup;

  boost::system::error_code er;
  socket_.assign(boost::asio::ip::tcp::v4(), context_->c.fd, er);
  if (er) {
    BB_ERROR_LOG("boost socket assign redis fd error:%s", er.message().c_str());
    redisAsyncFree(context_);
    context_ = nullptr;
    return false;
  }

  isRead_ = true;
  isWrite_ = true;
  waitRead();
  waitWrite();
  return true;
}

void AsyncRedisIf::SetConnectCallback(AsyncRedisIf::ConnectCallback cb) {
  connectCallback_ = std::move(cb);
}

bool AsyncRedisIf::IsConnected() {
  return context_ != nullptr;
}

void AsyncRedisIf::Disconnect() {
  if (context_ != nullptr) {
    redisAsyncDisconnect(context_);
  }
}

void AsyncRedisIf::SetDisconnectCallback(AsyncRedisIf::DisconnectCallback cb) {
  disconnectCallback_ = std::move(cb);
}

void AsyncRedisIf::RedisConnectCallback(const struct redisAsyncContext* c, int status) {
  auto ptr = reinterpret_cast<AsyncRedisIf*>(c->data);
  if (ptr == nullptr) return;

  if (status == 0) {
    if (ptr->connectCallback_) ptr->connectCallback_(status);
  } else {
    ptr->context_ = nullptr;
    BB_ERROR_LOG("async redis connect error:%s", c->errstr);
  }
}

void AsyncRedisIf::RedisDisconnectCallback(const struct redisAsyncContext* c, int status) {
  auto ptr = reinterpret_cast<AsyncRedisIf*>(c->data);
  if (ptr == nullptr) return;

  if (status == 0) {
    if (ptr->disconnectCallback_) ptr->disconnectCallback_(status);
  } else {
    ptr->context_ = nullptr;
    BB_ERROR_LOG("async redis disconnect error:%s", c->errstr);
  }
}

void AsyncRedisIf::waitRead() {
  if (!isRead_ || context_ == nullptr || isReading_) return;

  isReading_ = true;
  socket_.async_wait(boost::asio::ip::tcp::socket::wait_read, [this](const boost::system::error_code& ec) {
    isReading_ = false;
    if (ec && ec != boost::system::errc::operation_canceled) {
      BB_ERROR_LOG("async redis read error:%s", ec.message().c_str());
      return;
    }

    if (context_ != nullptr) {
      redisAsyncHandleRead(context_);
    }
  });
}

void AsyncRedisIf::waitWrite() {
  if (!isWrite_ || context_ == nullptr || isWriting_) return;

  isWriting_ = true;
  socket_.async_wait(boost::asio::ip::tcp::socket::wait_write, [this](const boost::system::error_code& ec) {
    isWriting_ = false;
    if (ec && ec != boost::system::errc::operation_canceled) {
      BB_ERROR_LOG("async redis write error:%s", ec.message().c_str());
      return;
    }

    if (context_ != nullptr) {
      redisAsyncHandleWrite(context_);
    }
  });
}

void AsyncRedisIf::RedisAddRead(void* privdata) {
  auto ptr = reinterpret_cast<AsyncRedisIf*>(privdata);
  if (ptr == nullptr || ptr->context_ == nullptr) return;

  ptr->isRead_ = true;
  ptr->waitRead();
}

void AsyncRedisIf::RedisDelRead(void* privdata) {
  auto ptr = reinterpret_cast<AsyncRedisIf*>(privdata);
  if (ptr == nullptr || ptr->context_ == nullptr) return;

  ptr->isRead_ = false;
}

void AsyncRedisIf::RedisAddWrite(void* privdata) {
  auto ptr = reinterpret_cast<AsyncRedisIf*>(privdata);
  if (ptr == nullptr || ptr->context_ == nullptr) return;

  ptr->isWrite_ = true;
  ptr->waitWrite();
}

void AsyncRedisIf::RedisDelWrite(void* privdata) {
  auto ptr = reinterpret_cast<AsyncRedisIf*>(privdata);
  if (ptr == nullptr || ptr->context_ == nullptr) return;

  ptr->isWrite_ = false;
}

void AsyncRedisIf::RedisCleanup(void* privdata) {
  auto ptr = reinterpret_cast<AsyncRedisIf*>(privdata);
  if (ptr == nullptr || ptr->context_ == nullptr) return;
  ptr->socket_.cancel();
  ptr->socket_.release();
  ptr->context_ = nullptr;
}

void AsyncRedisIf::AsyncCommand(void* data, const char* format, ...) {
  if (context_ == nullptr) return;

  va_list ap;
  va_start(ap, format);
  AsyncVCommand(data, format, ap);
  va_end(ap);
}

void AsyncRedisIf::AsyncVCommand(void* data, const char* format, va_list ap) {
  if (context_ == nullptr) return;

  redisvAsyncCommand(context_, &AsyncRedisIf::CommandHandle, data, format, ap);
}

void AsyncRedisIf::CommandHandle(struct redisAsyncContext* e, void* r, void* data) {
  if (e == nullptr || r == nullptr) return;
  auto ptr = reinterpret_cast<AsyncRedisIf*>(e->data);
  auto reply = reinterpret_cast<redisReply*>(r);
  ptr->AsyncCommandCallback(reply, data);
}

}
}