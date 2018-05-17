#include <iostream>

#include <bamboo/bamboo.hpp>

class EchoServer : public bamboo::server::ServerIf {
 public:
  using bamboo::server::ServerIf::ServerIf;
  virtual ~EchoServer() {}

  std::shared_ptr<bamboo::distributed::Registry> reg;

 protected:
  bool PrepareStart() override {
    auto acceptor = CreateAcceptor<bamboo::net::SimpleAcceptor>(ip_, port_);
    mgr_ = acceptor->CreateConnManager<bamboo::net::SimpleConnManager>();
    protocol_ = std::make_shared<bamboo::protocol::EchoProtocol>();
    mgr_->SetReadHandler([this](bamboo::net::SocketPtr so, const char* data, std::size_t size) -> std::size_t {
      total_ += size;
      return protocol_->ReceiveData(so, data, size);
    });

    GetScheduler().Heartbeat([this] {
      std::cout << "receive data:" << total_ / 1024.0 / 1024.0 << " m/10s" << std::endl;
      total_ = 0;
    }, 10000);

    return true;
  }

  bool FinishStart() override {
    reg->SetServerInfoHandler([this]()->std::string {
      std::string msg = ip_;
      msg.append(":").append(std::to_string(port_));
      return msg;
    });
    return true;
  }
  void StopHandle() override {}
  void Configure(boost::program_options::variables_map& map) override {
    if (map.count("ip")) {
      ip_ = map["ip"].as<std::string>();
    }
    if (map.count("port")) {
      port_ = map["port"].as<uint16_t>();
    }
  }

 private:
  bamboo::net::ConnManagerPtr mgr_;
  std::shared_ptr<bamboo::protocol::EchoProtocol> protocol_;
  std::size_t total_{0};
  std::string ip_;
  uint16_t port_{0};
};

int main(int argc, char* argv[]) {
  boost::program_options::variables_map vm;
  try {
    boost::program_options::options_description desc("echo server option");
    desc.add_options()
        ("help,h", "print all help manuals")
        ("ip,i", boost::program_options::value<std::string>()->required(), "server listen ip")
        ("port,p", boost::program_options::value<uint16_t>()->required(), "server listen port");

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
  auto reg = aio->InitRegistry();
  reg->Init("0.0.0.0:2181");
  reg->SetServerType("echo-server", 0, bamboo::distributed::NodeMode::MASTER_MASTER);
  auto server = aio->CreateServer<EchoServer>("echo_server");
  server.first->reg = reg;
  aio->Configure(vm);
  aio->Start();
  bamboo::env::Close();
  return 0;
}