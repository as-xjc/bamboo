#include "bamboo/env.hpp"

#include <boost/dll.hpp>

#include "bamboo/aio/multiaio.hpp"
#include "bamboo/aio/aio.hpp"

namespace {

bamboo::aio::AioPtr Aio;
std::unique_ptr<bamboo::schedule::Scheduler> scheduler;
std::string program_name;

}

namespace bamboo {
namespace env {

void Init(ThreadMode mode) {
  BB_ASSERT(Aio == nullptr);
  program_name = boost::dll::program_location().filename().string();
  BB_LOG_OPEN(program_name.c_str(), LOG_PID, LOG_LOCAL0);
  BB_DEBUG_LOG("open log:%s", program_name.c_str());

  if (mode == ThreadMode::SINGLE) {
    Aio = std::make_shared<bamboo::aio::Aio>();
  } else {
    Aio = std::make_shared<bamboo::aio::MultiAio>();
  }

  scheduler.reset(new bamboo::schedule::Scheduler(Aio->GetMasterIo()));
}

bamboo::aio::AioPtr GetIo() {
  return Aio;
}

bamboo::schedule::Scheduler& GetScheduler() {
  return *scheduler;
}

void Close() {
  BB_LOG_CLOSE();
}

}
}