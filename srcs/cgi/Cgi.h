#ifndef CGI_H
#define CGI_H
#include <string>
#include "config/ConfigServer.h"
#include "io/Connection.h"
#include "http/Request.h"
#include "http/Response.h"

class Cgi {
 public:
  Cgi(const ConfigServer &config_server, Connection &conn);
  ~Cgi() = default;
  bool act(const std::string& headers);
  static std::string getScriptName(const std::string& path);
  static char** makeEnv(const Request& req, const std::string& p, const std::string& addr, const ConfigServer& cfg);
  static void delEnv(char** arr);
  static void makeErrorResponse(Connection& connection, const ConfigServer& cfg);

 private:
  bool makeDocumentResponse_(const std::string& headers);
  bool makeLocalRedirResponse_(const std::string& headers);
  bool makeClientRedirResponse_(const std::string& headers);
  std::string findHeaderValue_(const std::string& headers, const std::string& key);
  void bufferResponse_(const Response& res);

  const ConfigServer& cfg_;
  Connection& conn_;
};

#endif
