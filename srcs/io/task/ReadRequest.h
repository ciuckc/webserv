#pragma once

#include <map>

#include "IOTask.h"
#include "http/Request.h"

class ConfigServer;

class ReadRequest : public ITask {
 private:
  enum State { ABORT = -1, DONE = 0, MSG, HEADERS };

  Request request_;
  State state_ = MSG;
  int error_ = 0;
  size_t req_len_ = 0;
  std::string header_key_;
  ConfigServer* cfg_ = nullptr;

  // return error code, or 0 if no error
  int use_line(Connection& connection, std::string& line);
  int handle_msg(Connection&, std::string& line);
  int handle_header(Connection& connection, std::string& line);
  std::pair<std::string, std::string_view> split_header(std::string& line);
  void error(Connection& connection);

  bool checkError(Connection&);

 public:
  explicit ReadRequest() = default;
  WS::IOStatus operator()(Connection& connection) override;
  void onDone(Connection& connection) override;

 private:

  using header_lambda = int(*)(ReadRequest&, const std::string_view&, Connection&);
  using header_lambda_map = std::multimap<std::string, header_lambda, WS::CaseCmpL> ;
  static const header_lambda_map hhooks_;
};
