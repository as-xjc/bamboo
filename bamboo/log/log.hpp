#pragma once

#include <syslog.h>

namespace bamboo {
namespace log {

/// 获取当前的日志记录等级
int GetLevel();

/// 日志等级转名字
const char* Level2Name(int priority);

/// 设置日志记录的等级
void SetLevel(int priority);

}
}

#define BB_LOG_LOCATION_FMT "<%s %d> "
#ifdef __CMAKE_FILE__
#define BB_LOG_LOCATION_ARGS __CMAKE_FILE__, __LINE__
#else
#define BB_LOG_LOCATION_ARGS __FILE__, __LINE__
#endif

#define BB_LOG_OPEN(ident, logopt, facility) ::openlog(ident, logopt, facility)
#define BB_LOG_CLOSE() ::closelog()
#define BB_LOG_RAW(priority, message, ...) ::syslog(priority, message, ##__VA_ARGS__)
#define BB_LOG(priority, message, ...) ::syslog(priority, BB_LOG_LOCATION_FMT message, BB_LOG_LOCATION_ARGS, ##__VA_ARGS__)

#define BB_LEVEL_LOG(priority, message, ...) \
do { \
  if (priority <= bamboo::log::GetLevel()) { \
    BB_LOG(priority, message, ##__VA_ARGS__); \
  } \
} while(false)

#define BB_INFO_LOG(message, ...) BB_LEVEL_LOG(LOG_INFO, message, ##__VA_ARGS__)
#define BB_ERROR_LOG(message, ...) BB_LEVEL_LOG(LOG_ERR, message, ##__VA_ARGS__)
#define BB_DEBUG_LOG(message, ...) BB_LEVEL_LOG(LOG_DEBUG, message, ##__VA_ARGS__)