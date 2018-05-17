#include "bamboo/utility/timemeasure.hpp"

#include "bamboo/log/log.hpp"

namespace bamboo {
namespace utility {

TimeMeasure::TimeMeasure(const char* name) : name_(name) {
  start_ = std::chrono::system_clock::now();
}

TimeMeasure::~TimeMeasure() {
  auto diff = std::chrono::system_clock::now() - start_;
  BB_LOG_RAW(LOG_INFO, "[%s] use %lld ms", name_.c_str(),
             std::chrono::duration_cast<std::chrono::milliseconds>(diff).count());
}

}
}