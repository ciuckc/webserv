#pragma once

#include "Handler.h"
#include "Socket.h"
#include "config/ConfigServer.h"

/**
 * Event subscriber that creates new Connection objects (which are Subscribers as well)
 */
class ListenSocket : public Handler {
 private:
  Server& server_;
  Socket socket_;
  std::map<std::string, ConfigServer&> configs_;

 protected:
  bool handleTimeout(Server& server, bool src) override;
  bool handleRead() override;
  bool handleWrite() override;
  bool handleWHup() override;
  bool handleRHup() override;
  bool handleError() override;

  void updateTimeout(const timep_t&) override;
  [[nodiscard]] const timep_t& getExpiry() const override;
  [[nodiscard]] const std::string& getName() const override;

    public:
  // This is necessary as `socket_' would otherwise be constructed after the Subscriber ctor, giving it the wrong fd
  // This function may throw IOExceptions, if binding fails etc
  ListenSocket(Server& server, uint16_t port);
  ListenSocket(ListenSocket&& s) = default;
  ~ListenSocket() override = default;

  // Map all hostnames for this config to itself, connections get a reference to these
  void addConfig(ConfigServer& srvcfg);
};
