#include <bamboo/bamboo.hpp>
#include <redis-asio/asyncredis.hpp>
#include <redis-asio/asyncredissubscriber.hpp>
#include <iostream>

int main(int argc, char* argv[]) {
  boost::program_options::variables_map vm;
  std::string ip;
  uint16_t port;
  try {
    boost::program_options::options_description desc("async redis option");
    desc.add_options()
        ("help,h", "print all help manuals")
        ("ip,i", boost::program_options::value<std::string>(&ip)->required(), "server listen ip")
        ("port,p", boost::program_options::value<uint16_t>(&port)->required(), "server listen port");

    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return EXIT_SUCCESS;
    }
    boost::program_options::notify(vm);
  } catch (std::exception& e) {
    std::cout << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  bamboo::env::Init();
  DEFER(bamboo::env::Close());
  auto aio = bamboo::env::GetIo();

  redis_asio::AsyncRedis redis(aio->GetMasterIo());
  redis.SetAddress(ip.c_str(), port);
  redis.Connect();
  int i = 0;

  redis_asio::AsyncRedisSubscriber subscriber(aio->GetMasterIo());
  subscriber.SetAddress(ip.c_str(), port);
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
