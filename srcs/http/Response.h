#pragma once
#include <ostream>
#include <string>

#include "Headers.h"
#include "Message.h"
#include "io/Socket.h"

class Response : public Message {
 public:
  Response();
  ~Response();
  Response(const Response& other);
  Response& operator=(const Response& rhs);

  void setMessage(int status);

  void write(Socket& socket) const;
};
