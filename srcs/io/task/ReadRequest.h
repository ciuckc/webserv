#pragma once
#include "IOTask.h"
#include <map>

class ReadRequest : public ITask {
 private:
  enum State {
    MSG,
    HEADERS,
    BODY,
    DONE,
  };

  Request request_;
  State state_ = MSG;
  int error_ = 0;
  size_t req_len_ = 0;
  std::string header_key_;
  ConfigServer* cfg_ = nullptr;

  bool use_line(Connection& connection, std::string& line);
  bool handle_msg(Connection&, std::string& line);
  bool handle_header(Connection& connection, std::string& line);
  bool handle_body(Connection& connection, std::string& line);
  std::pair<std::string, std::string_view> split_header(std::string& line);

 public:
  explicit ReadRequest() = default;
  bool operator()(Connection& connection) final;
  void onDone(Connection& connection) final;

 private:

  using header_lambda = int(*)(ReadRequest&, const std::string_view&, Connection&);
  using header_lambda_map = std::multimap<std::string, header_lambda, WS::CaseCmpL> ;
  static const header_lambda_map hhooks_;
};
