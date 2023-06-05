#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include "Server.h"
#include "config/Config.h"
#include "http/Request.h"
#include "http/ErrorResponse.h"
#include "io/EventQueue.h"
#include "io/Socket.h"

#define DEFAULT_CONFIG_FILE "./webserv.conf"

int main() {
  EventQueue queue;
  Socket socket;
  socket.bind(NULL, "6969");
  socket.listen(64);
  queue.add(socket.get_fd(), &socket, true);
  while (true) {
    EventQueue::event event = queue.getNext();
    EventQueue::Data& data = *(reinterpret_cast<EventQueue::Data*>(event.udata));
    if (data.context == &socket) {
      int handler = socket.accept();
      ErrorResponse status(404);
      std::stringstream strst;
      strst << status;
      std::string s = strst.str();
      size_t idx = 0;
      size_t len = s.length();
      while (idx < len) {
        idx += write(handler, s.c_str() + idx, len - idx);
      }
    }
  }

  std::ifstream str("../header.txt");
  Request req;
  str >> req;
  std::cout << req;
}