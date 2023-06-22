#pragma once
#include "IOTask.h"

class SendResponse : public OTask {
 private:
  enum State {
    MSG,
    HEADERS,
    SEPARATOR,
    BODY
  };

  typedef Message::header_t::const_iterator header_iter_t;
  const Response response_;
  State state_ = MSG;
  header_iter_t header_;

 public:
  explicit SendResponse(const Response& response);
  bool operator()(Connection& connection) override;
  void onDone(Connection& connection) final;
};
