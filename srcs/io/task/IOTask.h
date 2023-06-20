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

// Input task
class ITask : public IOTask {};
// Output task
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
  explicit SendResponse(const Response& response);
  ~SendResponse() override = default;

  bool operator()(Connection& connection) override;
  void onDone(Connection& connection) final;
};

class ReadRequest : public ITask {
 public:
  bool operator()(Connection& connection) final;
  void onDone(Connection& connection) final;
};
