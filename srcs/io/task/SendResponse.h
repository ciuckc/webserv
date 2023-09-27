#pragma once

#include "IOTask.h"
#include "http/Response.h"

class SendResponse : public OTask {
 private:
  enum State {
    MSG,
    HEADERS,
    SEPARATOR
  };

  typedef Message::header_t::const_iterator header_iter_t;
  const Response response_;
  State state_ = MSG;
  header_iter_t header_;

 public:
  explicit SendResponse(Response&& response);
  WS::IOStatus operator()(Connection& connection) override;
  void onDone(Connection& connection) final;
};
