#include <bamboo/bamboo.hpp>
#include <iostream>

int main(int argc, char* argv[]) {
  bamboo::env::Init();
  DEFER(bamboo::env::Close());
  auto aio = bamboo::env::GetIo();

  bamboo::cache::AsyncRedis redis(aio->GetMasterIo());
  redis.SetAddress("0.0.0.0", 6379);
  redis.Connect();
  int i = 0;

  bamboo::cache::AsyncRedisSubscriber subscriber(aio->GetMasterIo());
  subscriber.SetAddress("0.0.0.0", 6379);
  subscriber.Connect();

  subscriber.Subscribe("test.new", [](std::string channel, std::string message) {
    std::cout << "Subscribe <--:" << channel << " " << message << std::endl;
  });

  subscriber.PSubscribe("test.*", [](std::string channel, std::string message) {
    std::cout << "test.* <--:" << channel << " " << message << std::endl;
  });
  subscriber.PSubscribe("test.n*", [](std::string channel, std::string message) {
    std::cout << "test.n* <--:" << channel << " " << message << std::endl;
  });

  bamboo::env::GetScheduler().Heartbeat([&]() {
    redis.Publish("test.new", std::to_string(i++));
  }, 1000);

  bamboo::env::GetScheduler().Timeout([&]() {
    aio->Stop();
  }, 60000);
  aio->Start();
  return 0;
}
