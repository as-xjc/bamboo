#pragma once

#include <memory>

namespace bamboo {
namespace protocol {

/**
 * 消息基类
 */
class MessageIf {
 public:

  /**
   * 生成数据对象
   * @return 二进制数据的共享指针对象
   */
  virtual std::unique_ptr<std::string> Build() = 0;
};

}
}