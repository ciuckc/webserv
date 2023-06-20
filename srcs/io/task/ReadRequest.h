#pragma once
#include "IOTask.h"

class ReadRequest : public ITask {
 private:
  enum State {
    MSG,
    HEADERS,
    BODY,
  };

  Request& request_;
  State state_ = MSG;
  int error_ = 0;

  bool use_line(std::string& line);

 public:
  explicit ReadRequest(Request& req);
  bool operator()(Connection& connection) final;
  void onDone(Connection& connection) final;
};