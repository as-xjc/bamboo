#pragma once

#include <memory>

#include "bufferif.hpp"

namespace bamboo {
namespace buffer {

const std::size_t INIT_CAPACITY = 128;

class DynamicBuffer : public BufferIf {
 public:
  DynamicBuffer();
  virtual ~DynamicBuffer();

  std::size_t Write(const char* data, std::size_t size) override;
  std::size_t Read(char* data, std::size_t size, bool skip = true) override;
  std::size_t Size() override;
  std::size_t Capacity() override;
  char* Data() override;
  std::size_t Free() override;
  void Skip(std::size_t size, SkipType type = SkipType::READ) override;

 private:
  void Grow(std::size_t need);
  char* Head();
  char* Tail();
  std::unique_ptr<char[]> buffer_;
  std::size_t head_{0};
  std::size_t tail_{0};
  std::size_t capacity_{0};
};

}
}

