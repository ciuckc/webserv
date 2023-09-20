#include "ListenSocket.h"

#include <sys/socket.h>

#include "Connection.h"
#include "Handler.h"
#include "Server.h"

using namespace std::chrono_literals;
ListenSocket::ListenSocket(Server& server, uint16_t port)
    : Handler(-1, EventQueue::in, 0), server_(server), socket_() {
  fd_ = socket_.get_fd();
  socket_.bind(port);
  socket_.listen(SOMAXCONN);
}

const ListenSocket::timep_t& ListenSocket::getExpiry() const {
  static const auto maxt = std::chrono::system_clock::time_point::max();
  return maxt;
}
void ListenSocket::updateTimeout(const timep_t&) {}
bool ListenSocket::handleTimeout(Server&, bool) {
  abort();
}

bool ListenSocket::handleRead() {
  server_.add_sub(
      std::make_unique<Connection>(server_, std::forward<Socket>(socket_.accept()), configs_));
  return false;
}

bool ListenSocket::handleWrite() {
  Log::warn(*this, "Unhandled event on listen socket\n");
  return false;
}

bool ListenSocket::handleWHup() {
  Log::warn(*this, "Unhandled event on listen socket\n");
  return false;
}

bool ListenSocket::handleRHup() {
  Log::warn(*this, "Unhandled event on listen socket\n");
  return false;
}

bool ListenSocket::handleError() {
  Log::warn(*this, "Unhandled event on listen socket\n");
  return false;
}

void ListenSocket::addConfig(ConfigServer& srvcfg) {
  auto& hostnames = srvcfg.getHostnames();
  if (hostnames.empty()) {
    if (!configs_.try_emplace("", srvcfg).second)
      Log::warn("Trying to add duplicate host:port config :", srvcfg.getPort(), " to map!\n");
    return;
  }
  for (auto& name : hostnames) {
    auto pos = configs_.find(name);
    if (pos != configs_.end())
      Log::warn("Trying to add duplicate host:port config ", name, ':', srvcfg.getPort(), " to map!\n");
    else
      configs_.emplace(name, srvcfg);
  }
}

const std::string& ListenSocket::getName() const {
  return socket_.getName();
}
