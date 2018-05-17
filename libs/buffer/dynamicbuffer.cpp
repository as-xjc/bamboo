#include "bamboo/buffer/dynamicbuffer.hpp"

namespace bamboo {
namespace buffer {

DynamicBuffer::DynamicBuffer() {
  buffer_.reset(new char[INIT_CAPACITY]);
  capacity_ = INIT_CAPACITY;
}

DynamicBuffer::~DynamicBuffer() {}

std::size_t DynamicBuffer::Write(const char* data, std::size_t size) {
  if (Free() < size) Grow(size);

  std::memcpy(Tail(), data, size);
  Skip(size, SkipType::WRITE);
  return size;
}

std::size_t DynamicBuffer::Read(char* data, std::size_t size, bool skip) {
  std::size_t readSize = size;
  if (readSize > Size()) readSize = Size();

  std::memcpy(data, Head(), readSize);
  if (skip) Skip(readSize, SkipType::READ);
  return readSize;
}

std::size_t DynamicBuffer::Free() {
  return Capacity() - tail_;
}

void DynamicBuffer::Skip(std::size_t size, bamboo::buffer::SkipType type) {
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

std::size_t DynamicBuffer::Size() {
  return tail_ - head_;
}

std::size_t DynamicBuffer::Capacity() {
  return capacity_;
}

char* DynamicBuffer::Head() {
  return &(buffer_.get()[head_]);
}

char* DynamicBuffer::Tail() {
  return &(buffer_.get()[tail_]);
}

void DynamicBuffer::Grow(std::size_t need) {
  std::size_t newSize = Capacity() + need;
  std::size_t calcSize = Capacity();
  while (calcSize < newSize) calcSize *= 2;

  std::unique_ptr<char[]> newbuff(new char[calcSize]);
  std::memcpy(newbuff.get(), Head(), Size());

  tail_ = Size();
  head_ = 0;
  capacity_ = calcSize;
  buffer_ = std::move(newbuff);
}

char* DynamicBuffer::Data() {
  return Head();
}

}
}