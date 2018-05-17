#pragma once

#include <string>
#include <chrono>

namespace bamboo {
namespace utility {

/**
 * 时间测量工具类
 */
class TimeMeasure final {
 public:
  TimeMeasure(const char* name);
  ~TimeMeasure();

 private:
  std::string name_;
  std::chrono::system_clock::time_point start_;
};

}
}

