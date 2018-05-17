#include <iostream>

#include <bamboo/bamboo.hpp>

int main(int argc, char* argv[]) {
  bamboo::env::Init();
  auto aio = bamboo::env::GetIo();

  /**
   * 服务注册
   */
  auto reg = aio->InitRegistry();
  reg->Init("0.0.0.0:2181");
  reg->SetServerType("echo-client", 0, bamboo::distributed::NodeMode::MASTER_MASTER);
  // 注册关注服务器
  reg->AddWatchServer("echo-server", 0);

  auto server = aio->CreateServer<bamboo::server::SimpleServer>("echo-client");
  auto connect = server.first->CreateConnector<bamboo::net::SimpleConnector>();
  auto manager = connect->CreateConnManager<bamboo::net::SimpleConnManager>();
  manager->SetReadHandler([](bamboo::net::SocketPtr so, const char* data, std::size_t size) -> std::size_t {
    so->WriteData(data, size);
    return size;
  });

  /**
   * 获取到一个新加的服务器,链接他,并定时3秒后开始发数据
   */
  reg->SetAddServerHandler([=](std::string type, int zone, std::string id, std::string info) {
    printf("get server %s,%d,%s,%s\n", type.c_str(), zone, id.c_str(), info.c_str());
    
    auto it = info.find(':');
    if (it == std::string::npos) return;
    std::string ip = info.substr(0, it);
    std::string portstr = info.substr(it+1, info.length());
    int port = std::stoi(portstr);

    auto socket = connect->Connect(ip, port);
    if (socket) {
      server.first->GetScheduler().Timeout([socket]() {
        std::string t = std::to_string(std::time(nullptr));
        std::cout << "send data:" << t << std::endl;
        socket->WriteData(t.data(), t.size());
      }, 3000);
    }
  });
  aio->Start();
  return 0;
}