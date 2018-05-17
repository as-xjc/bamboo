#include "bamboo/log/log.hpp"

namespace {
int log_level = LOG_DEBUG;
}

namespace bamboo {
namespace log {

int GetLevel() {
  return log_level;
}

const char* Level2Name(int priority) {
  switch (priority) {
    case LOG_DEBUG: return "debug";
    case LOG_INFO: return "info";
    case LOG_WARNING: return "warn";
    case LOG_ERR: return "error";
  }
  return "unknow";
}

void SetLevel(int priority) {
  log_level = priority;
}

}
}