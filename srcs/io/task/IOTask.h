#pragma once

#include <string>
#include <iostream>

#include "io/Connection.h"
#include "http/ErrorResponse.h"

class IOTask {
 protected:
  virtual ~IOTask() = default;
 public:
  virtual bool operator()(Connection& connection) = 0;
  virtual void onDone(Connection& connection) = 0;
};

class ITask : public IOTask {};
class OTask : public IOTask {};

// This class could probably be just a more specialized version of sendrequest (once it exists)
class SendResponse : public OTask {
 private:
  enum State { MSG, HEADERS, SEPARATOR, BODY };

 private:
  typedef Message::header_t::const_iterator header_iter_t;
  const Response response_;

  State state = MSG;
  header_iter_t header_;
 public:
  explicit SendResponse(const Response& response)
    : response_(response),
      header_(response_.getHeaders().begin()) {}
  ~SendResponse() override = default;

  bool operator()(Connection& connection) override {
    while (!connection.getBuffer().needWrite()) {
      switch (state) {
        case MSG:
          connection.getBuffer() << response_.getMessage();
          state = HEADERS;
          break;
        case HEADERS:
          connection.getBuffer() << *header_++;
          if (header_ == response_.getHeaders().end())
            state = SEPARATOR;
          break;
        case SEPARATOR:
          connection.getBuffer() << "\r\n";
          if (response_.getBodySize() == 0)
            return true;
          state = BODY;
          break;
        case BODY:
          connection.getBuffer() << response_.getBody();
          return true;
      }
    }
    return false;
  }
  void onDone(Connection& connection) final {
    //todo: Add task to read next request!
    connection.close();
  }
};

class ReadRequest : public ITask {
 public:
  bool operator()(Connection& connection) final {
    std::string line;
    while (!connection.getBuffer().getline(line).readFailed()) {
      if (line == "\r\n")
        return true;
      std::cout << line;
    }
    return false;
  }
  void onDone(Connection& connection) final {
    connection.addTask(new SendResponse(ErrorResponse(404)));
  }
};
