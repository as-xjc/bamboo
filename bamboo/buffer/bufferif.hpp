#pragma once

#include <cstdint>

namespace bamboo {
namespace buffer {

enum class SkipType { WRITE, READ };

class BufferIf {
 public:
  virtual std::size_t Write(const char* data, std::size_t size) = 0;
  virtual std::size_t Read(char* data, std::size_t size, bool skip = true) = 0;
  virtual std::size_t Size() = 0;
  virtual std::size_t Capacity() = 0;
  virtual char* Data() = 0;
  virtual std::size_t Free() = 0;
  virtual void Skip(std::size_t size, SkipType type = SkipType::READ) = 0;
};

}
}