#include "Response.h"

#include <cstring>
#include <sstream>

#include "Status.h"

Response::Response() {}

Response::~Response() {}

Response::Response(const Response &other) : Message(other) {}

Response &Response::operator=(const Response &rhs) {
  Message::operator=(rhs);
  return *this;
}

void Response::setMessage(int status) {
  std::stringstream str;
  str << "HTTP/1.1 " << status << " " << http::getStatus(status) << "\r\n";
  message_ = str.str();
}

void Response::write(Socket &socket) const {
  //size_t offs = 0;
  socket.write(message_);
  for (auto i = headers_.begin(); i < headers_.end(); ++i) {
    socket.write(*i);
  }
  //socket.write(headers_.begin(), headers_.end(), offs);
  socket.write("\r\n");
  if (body_) socket.write(body_);
}
