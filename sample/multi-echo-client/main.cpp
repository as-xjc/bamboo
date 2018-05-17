#include <iostream>

#include <bamboo/bamboo.hpp>

int main(int argc, char* argv[]) {
  std::string ip;
  std::uint16_t port;
  boost::program_options::variables_map vm;
  try {
    boost::program_options::options_description desc("multi-threads echo client option");
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

  bamboo::env::Init(bamboo::env::ThreadMode::MULTIPLE);
  auto aio = bamboo::env::GetIo();
  aio->BatchCreateServer<bamboo::server::SimpleServer>(aio->GetIoSize(), "echo-client");

  aio->ForeachServer([&ip, &port](bamboo::server::ServerPtr server) {
    auto connect = server->CreateConnector<bamboo::net::SimpleConnector>();
    auto manager = connect->CreateConnManager<bamboo::net::SimpleConnManager>();
    manager->SetReadHandler([](bamboo::net::SocketPtr so, const char* data, std::size_t size) -> std::size_t {
      so->WriteData(data, size);
      return size;
    });
    auto socket = connect->Connect(ip, port);
    if (socket) {
      server->GetScheduler().Timeout([socket]() {
        std::string t = std::to_string(std::time(nullptr));
        std::cout << "send data:" << t << std::endl;
        socket->WriteData(t.data(), t.size());
      }, 3000);
    } else {
      std::exit(EXIT_FAILURE);
    }
  });
  aio->Start();
  return 0;
}