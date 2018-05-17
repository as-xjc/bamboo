#pragma once

#include <array>

#include <bamboo/buffer/bufferif.hpp>

namespace bamboo {
namespace buffer {

template <std::size_t SIZE>
class FixedBuffer : public BufferIf {
 public:
  FixedBuffer() {}
  virtual ~FixedBuffer() {}

  std::size_t Write(const char* data, std::size_t size) override {
    std::size_t write = size;
    if (Free() < write) write = Free();
    if (write < 1) return 0;

    std::memcpy(Tail(), data, write);
    Skip(write, SkipType::WRITE);
    return write;
  }

  std::size_t Read(char* data, std::size_t size, bool skip = true) override {
    std::size_t read = size;
    if (Size() < read) read = Size();
    if (read < 1) return 0;

    std::memcpy(data, Head(), read);
    if (skip) Skip(read, SkipType::READ);
    return read;
  }

  std::size_t Size() override {
    return tail_ - head_;
  }

  std::size_t Capacity() override {
    return buffer_.size();
  }

  char* Data() override {
    return Head();
  }

  std::size_t Free() override {
    return Capacity() - tail_;
  }

  void Skip(std::size_t size, SkipType type = SkipType::READ) override {
    if (type == SkipType::READ) {
      head_ += size;
      if (head_ == tail_) {
        head_ = 0;
        tail_ = 0;
      }
    } else {
      tail_ += size;
    }
  }

  char* Head() {
    return &(buffer_.data()[head_]);
  }

  char* Tail() {
    return &(buffer_.data()[tail_]);
  }

 private:
  std::array<char, SIZE> buffer_;
  std::size_t head_{0};
  std::size_t tail_{0};
};

}
}

