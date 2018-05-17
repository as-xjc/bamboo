#include <bamboo/server/simpleserver.hpp>

namespace bamboo {
namespace server {

SimpleServer::~SimpleServer() {}

bool SimpleServer::PrepareStart() {
  return true;
}

bool SimpleServer::FinishStart() {
  return true;
}

void SimpleServer::StopHandle() {}

void SimpleServer::Configure(boost::program_options::variables_map& map) {}

}
}