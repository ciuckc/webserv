#include <iostream>
#include <unistd.h>
#include <sstream>
#include "Server.h"
#include "io/IOException.h"
#include "http/ErrorResponse.h"

Server::Server() {
  listen_socket_.bind(NULL, "6969");
  listen_socket_.listen(128);
  evqueue_.add(listen_socket_.get_fd(), &listen_socket_, IN);
}

Server::~Server() {}

static void send_error(EventQueue& queue, int fd, int error) {
  (void)queue;
  ErrorResponse err(error);
  std::stringstream strs;
  strs << err;
  std::string str = strs.str();
  write(fd, str.c_str(), str.size());
  close(fd);
}

static void do_read(EventQueue& queue, int fd) {
  char buf[4096];
  ssize_t amt = 4096;
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
  queue.mod(fd, (void*) send_error, OUT);
}

void Server::loop() {

  while (true) {
    EventQueue::Data& data = evqueue_.getNext();
    if (data.fd == listen_socket_.get_fd()) {
      evqueue_.add(listen_socket_.accept(), (void*)do_read, IN);
    } else if (data.handler == do_read) {
      do_read(evqueue_, data.fd);
    } else if (data.handler == send_error) {
      send_error(evqueue_, data.fd, 418);
    }

    data();
  }
}
