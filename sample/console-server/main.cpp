#include <iostream>

#include <bamboo/bamboo.hpp>

int main(int argc, char* argv[]) {
  bamboo::env::Init();
  auto aio = bamboo::env::GetIo();
  auto server = aio->CreateServer<bamboo::server::Console>("console-server").first;
  server->SetAddress("0.0.0.0", 4444);
  server->RegCmd("hello", [](std::vector<std::string>& args)->std::string {
    std::string hello = "hello, world";
    return hello;
  }, "print hello");
  aio->Start();
  bamboo::env::Close();
  return 0;
}