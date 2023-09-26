#pragma once

#include "Request.h"
#include "Response.h"
#include "config/ConfigRoute.h"
#include "io/Connection.h"

class RequestHandler {
 public:
  RequestHandler(Connection& connection, const ConfigServer& cfg, Request& req)
    : connection_(connection), cfg_(cfg), request_(req) {}
  ~RequestHandler() = default;
  RequestHandler(const RequestHandler& that) = delete;
  RequestHandler& operator=(const RequestHandler& that) = delete;
  void execRequest(const std::string& path, const ConfigRoute& route);
  void handleError_(int err); // public so we can send errors more easily

 private:
  using FileInfo = util::FileInfo;
  Connection& connection_;

  const ConfigServer& cfg_;
  Request&   request_;

  void handleDir_(const std::string& path, const ConfigRoute& route, FileInfo& file_info);
  void handleFile_(const std::string& path, FileInfo& file_info);
  void handleRedir_(const ConfigRoute& route);
  void handlePut_(const std::string& path, const ConfigRoute& route);
  void deleteFile_(const std::string& path);
  void handleCgi_(const std::string& path);
  void autoIndex_(const std::string& path);
  void handleFileError_();
};
