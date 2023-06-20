#pragma once
#include <ostream>
#include <string>

#include "Headers.h"
#include "io/Socket.h"
#include "Message.h"

class Response : public Message {
 public:
  Response();
  ~Response() override;
  Response(const Response& other);
  Response& operator=(const Response& rhs);

  void setMessage(int status);

  void write(Socket& socket) const;
};
