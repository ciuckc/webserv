#pragma once
#include <ostream>
#include <string>

#include "Message.h"
#include "io/Socket.h"

class Response : public Message {
 public:
  Response();
  ~Response() override;
  Response(const Response& other);
  Response& operator=(const Response& rhs);

  void setMessage(int status);
  void setKeepAlive(uint32_t timeout, uint32_t max_requests);
};
