#include "bamboo/net/socket.hpp"

namespace bamboo {
namespace net {

Socket::~Socket() {}

void Socket::ReadData() {
  if (buffer_.Free() < 1) {
    BB_ERROR_LOG("socket read buffer is full????");
    return;
  }
  auto self = shared_from_this();
  async_read_some(boost::asio::buffer(buffer_.Tail(), buffer_.Free()),
                  [this, self](const boost::system::error_code& ec, std::size_t rd) {
                    if (ec) {
                      BB_ERROR_LOG("socket[%d] read data ec:%s", GetId(), ec.message().c_str());
                      Close();
                      return;
                    }
                    buffer_.Skip(rd, bamboo::buffer::SkipType::WRITE);
                    auto& handler = GetReadHandler();
                    if (handler) {
                      std::size_t read = handler(buffer_.Head(), buffer_.Size());
                      buffer_.Skip(read, bamboo::buffer::SkipType::READ);
                    } else {
                      buffer_.Skip(rd, bamboo::buffer::SkipType::READ);
                    }

                    ReadData();
                  });
}

void Socket::DoWriteData() {
  if (list_.empty()) return;

  auto buff = list_.front();
  auto self = shared_from_this();
  async_write_some(boost::asio::buffer(buff->data(), buff->size()),
                   [buff, self, this](const boost::system::error_code& ec, std::size_t wd) {
                     if (ec) {
                       BB_ERROR_LOG("socket[%d] write data ec:%s", GetId(), ec.message().c_str());
                       Close();
                       return;
                     }

                     if (wd < buff->size()) {
                       buff->erase(0, wd);
                     } else {
                       list_.pop_front();
                     }
                     DoWriteData();
                   });
}

void Socket::WriteData(const char* data, std::size_t size) {
  if (!is_open() || size < 1) return;

  auto buff = std::make_shared<std::string>();
  buff->append(data, size);

  if (list_.empty()) {
    list_.push_back(buff);
    DoWriteData();
  } else {
    list_.push_back(buff);
  }
}

void Socket::WriteData(bamboo::protocol::MessageIf* message) {
  if (!is_open()) return;

  WriteData(message->Build());
}

void Socket::WriteData(std::unique_ptr<std::string>&& buffer) {
  if (!is_open() || !buffer || buffer->size() < 1) return;

  std::shared_ptr<std::string> buff(buffer.release());

  if (list_.empty()) {
    list_.push_back(buff);
    DoWriteData();
  } else {
    list_.push_back(buff);
  }
}

void Socket::Close() {
  BB_DEBUG_LOG("socket close:%d", GetId());
  auto& closer = GetCloseHandler();
  if (closer) closer();

  if (is_open()) this->close();
}

}
}