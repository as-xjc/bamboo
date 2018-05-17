#include <iostream>

#include <bamboo/bamboo.hpp>

int main(int argc, char* argv[]) {
  std::string ip;
  std::uint16_t port;
  boost::program_options::variables_map vm;
  try {
    boost::program_options::options_description desc("echo client option");
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
  auto aio = bamboo::env::GetIo();
  auto server = aio->CreateServer<bamboo::server::SimpleServer>("echo-client");
  auto connect = server.first->CreateConnector<bamboo::net::SimpleConnector>();
  auto manager = connect->CreateConnManager<bamboo::net::SimpleConnManager>();
  manager->CreateProtocol<bamboo::protocol::EchoProtocol>();
  auto socket = connect->Connect(ip, port);
  if (socket) {
    server.first->GetScheduler().Timeout([socket]() {
      std::string t = std::to_string(std::time(nullptr));
      std::cout << "send data:" << t << std::endl;
      socket->WriteData(t.data(), t.size());
    }, 3000);
  } else {
    return EXIT_FAILURE;
  }
  aio->Start();
  return 0;
}