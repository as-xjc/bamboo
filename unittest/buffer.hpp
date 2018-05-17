#pragma once

#include <gtest/gtest.h>

#include <bamboo/buffer/dynamicbuffer.hpp>
#include <bamboo/buffer/fixedbuffer.hpp>

TEST(dynamicBuffer, Init) {
  bamboo::buffer::DynamicBuffer buffer;
  ASSERT_EQ(buffer.Capacity(), 128);
  ASSERT_EQ(buffer.Size(), 0);
  uint32_t text = 1;
  buffer.Write(reinterpret_cast<const char*>(&text), sizeof(text));
  ASSERT_EQ(buffer.Size(), sizeof(text));
}

TEST(FixedBuffer, Init) {
  bamboo::buffer::FixedBuffer<20> buffer;
  ASSERT_EQ(buffer.Capacity(), 20);
  ASSERT_EQ(buffer.Size(), 0);
}

TEST(FixedBuffer, Write) {
  bamboo::buffer::FixedBuffer<20> buffer;
  std::string nihao = "nihao";
  buffer.Write(nihao.data(), nihao.length());
  ASSERT_EQ(buffer.Size(), nihao.length());
  ASSERT_EQ(buffer.Free(), buffer.Capacity() - nihao.length());
}