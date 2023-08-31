#ifndef REQUESTHANDLER_H
#define REQUESTHANDLER_H
#include "Request.h"
#include "Response.h"
#include "io/Connection.h"

class RequestHandler {
 public:
  RequestHandler(Connection& connection, const ConfigServer& cfg, Request& req)
    : connection_(connection), cfg_(cfg), request_(req) {}
  ~RequestHandler() = default;
  RequestHandler(const RequestHandler& that) = delete;
  RequestHandler& operator=(const RequestHandler& that) = delete;
  void       execRequest();
  Response&& getResponse();

 private:
  using stat_t = struct stat;
  static const size_t buffer_size_ = 512;
  Connection& connection_;

  const ConfigServer& cfg_;
  const Request&   request_;
  Response  response_;

  // some kind of method that finds all servers listening to used socket
  // and returns the one with the server_name specified in the HOST header
  bool legalMethod_() const;
  bool isRedirect_() const;
  void handleDir_(std::string& path);
  void handleFile_(stat_t& file_info, std::string& path);
  void handleError_(int err);
  void autoIndex_(std::string& path);
};

#endif
