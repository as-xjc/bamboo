#pragma once

#include <bamboo/aio/aioif.hpp>
#include <bamboo/schedule/scheduler.hpp>

namespace bamboo {
namespace env {

/// Aio 模式
enum class ThreadMode {
  SINGLE, /**< 单线程模式 */
  MULTIPLE /**< 多线程模式 */
};

/// 初始化aio模式
void Init(ThreadMode mode = ThreadMode::SINGLE);

/// 获取aio
bamboo::aio::AioPtr GetIo();

/**
 * 获取调度器
 *
 * @note 这是一个全局的调度器，位于主io
 */
bamboo::schedule::Scheduler& GetScheduler();

/// 关闭环境，关闭aio，清理各类资源
void Close();

}
}