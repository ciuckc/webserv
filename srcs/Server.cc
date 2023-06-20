#include "Server.h"

#include <unistd.h>

#include <iostream>
#include <sstream>

#include "http/ErrorResponse.h"
#include "io/IOException.h"

Server::Server() {
  listen_socket_.bind(NULL, "6969");
  listen_socket_.listen(128);
  // This is weird for now, as the socket in here will share the file descriptor
  // with the socket inside the Data struct inside the event queue, so when
  // the event queue gets destructed this file descriptor will be invalidated
  // twice, in the socket here and in the queue.
  // Server::addListenSocket() could work? We're gonna need multiple anyway
  evqueue_.add(listen_socket_.get_fd(), &listen_socket_, IN);
}

Server::~Server() {}

static void do_read(EventQueue& queue, EventQueue::Data& data);
static void send_error(EventQueue& queue, EventQueue::Data& data, int error) {
  (void)queue;
  ErrorResponse err(error);
  err.write(data.socket);
  data.socket.flush();
  queue.mod(data.socket.get_fd(), (void*)do_read, IN);
}

static void do_read(EventQueue& queue, EventQueue::Data& data) {
  char buf[4096];
  ssize_t amt = 4096;
  int fd = data.socket.get_fd();
  while (true) {
    ssize_t readb = read(fd, buf + (4096 - amt), amt);
    if (readb < amt) {
      amt -= readb;
      std::cout.write(buf, 4096 - amt);
      break;
    }
    std::cout.write(buf, 4096);
    amt = 4096;
  }
  queue.mod(fd, (void*)send_error, OUT);
}

void Server::loop() {
  std::cout << "[Server] Entering main loop!\n";
  while (true) {
    try {
      EventQueue::Data& data = evqueue_.getNext();
      if (data.socket.get_fd() == listen_socket_.get_fd()) {
        evqueue_.add(listen_socket_.accept(), (void*)do_read, IN);
      } else if (data.handler == do_read) {
        do_read(evqueue_, data);
      } else if (data.handler == send_error) {
        send_error(evqueue_, data, 418);
      }

      data();
    } catch (const IOException& err) {
      std::cout << "IOException: " << err.what();
    }
  }
}
