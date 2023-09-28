#include "Cgi.h"

#include <unistd.h>

#include <array>

#include "config/ConfigServer.h"
#include "http/RequestHandler.h"
#include "util/WebServ.h"
#include "http/ErrorPage.h"

std::string Cgi::findHeaderValue_(const std::string& headers, const std::string& key)
{
  size_t start = headers.find(key) + key.length();
  size_t i = 0;
  while (!std::isspace(headers[start + i])) {
    i++;
  }
  return headers.substr(start, i);
}

void Cgi::delEnv(char** arr)
{
  size_t i = 0;
  if (!arr)
    return;
  while (arr[i]) {
    delete[] arr[i];
    i++;
  }
  delete[] arr;
}

// make environment variables as specified in CGI RFC
// some of them are missing due to not being required by subject
char** Cgi::makeEnv(const Request& req, const std::string& path, const std::string& addr, const ConfigServer& cfg)
{
  static struct header_null_helper {
    std::string operator()(const Request& req, const std::string& key) {
      const char* header = req.getHeader(key);
      if (!header) {
        return ("");
      }
      std::string str(header);
      size_t start = str.find(key) + key.length();
      size_t i = 0;
      while (!std::isspace(str[start + i])) {
        i++;
      }
      return str.substr(start, i);
    };
  } head;
  char** env;
  using namespace std::string_literals;
  std::string script = getScriptName(path);
  std::array<std::string, 16> arr = {
    "CONTENT_LENGTH="s + head(req, "Content-Length: "),
    "CONTENT_TYPE="s + head(req, "Content-Type: "),
    "GATEWAY_INTERFACE=CGI/1.1"s,
    "PATH_INFO="s + path.substr(script.length()),
    "PATH_TRANSLATED="s + script,
    "QUERY_STRING="s +
      ((req.getUri().find('?') != std::string::npos) ? req.getUri().substr(req.getUri().find('?') + 1) : ""),
    "REMOTE_ADDR="s + addr,
    "REMOTE_HOST="s + head(req, "Host: "),
    "REQUEST_METHOD="s + (req.getMethod() == HTTP::GET ? "GET" : "POST"),
    "SCRIPT_NAME="s + script,
    "SERVER_NAME="s + head(req, "Host: "),
    "SERVER_PORT="s + std::to_string(cfg.getPort()),
    "SERVER_PROTOCOL=HTTP/1.1"s,
    "SERVER_SOFTWARE=SuperWebserv10K/0.9.1 (Unix)"s
  };
  env = new char*[arr.size() + 1];
  env[arr.size()] = nullptr;
  for (size_t i = 0; i < arr.size(); i++) {
    env[i] = new char[arr[i].length() + 1];
    arr[i].copy(env[i], arr[i].length());
    env[i][arr[i].length()] = '\0';
  }
  return (env);
}

std::string Cgi::getScriptName(const std::string& path)
{
  return (path.substr(0, path.find(".cgi") + 4));
}

Cgi::Cgi(const ConfigServer &config_server, Connection &conn)
  : cfg_(config_server), conn_(conn) {}

bool Cgi::act(const std::string& headers)
{
  // document response
  if (strncasecmp("Content-Type", headers.c_str(), 12) == 0) {
    if (makeDocumentResponse_(headers))
      return true;
  }
  else if (strncasecmp("Location", headers.c_str(), 8) == 0) {
    if (headers.find("http://") != std::string::npos || headers.find("https://") != std::string::npos) {
      // client-redir response
      if (makeClientRedirResponse_(headers))
        return true;
    } else {
      // local-redir response
      if (makeLocalRedirResponse_(headers))
        return true;
    }
  }
  // invalid response (not compliant with CGI spec)
  auto perr = HTTP::createError(cfg_, 500);
  conn_.enqueueResponse(std::move(perr.first));
  conn_.addTask(std::move(perr.second));
  size_t len = conn_.getOutBuffer().dataLen();
  conn_.getOutBuffer().discard(len);
  return false;
}

// cgi document response into http response
bool Cgi::makeDocumentResponse_(const std::string& headers)
{
  Response res;
  res.addHeader("Transfer-Encoding", "Chunked");
  res.addHeader("Content-Type", findHeaderValue_(headers, "Content-Type: "));
  res.setMessage(200);
  bufferResponse_(res);
  return true;
}

// cgi local redirect response into http response
bool Cgi::makeLocalRedirResponse_(const std::string& headers)
{
  Request req;
  req.setMethod(HTTP::GET);
  req.setVersion(Request::VER_1_1);
  RequestHandler rh(conn_, cfg_, req);
  std::string new_uri = findHeaderValue_(headers, "Location: ");
  auto new_route = cfg_.matchRoute(new_uri);
  if (new_route != cfg_.getRoutes().end()) {
    req.setUri(new_uri);
    rh.execRequest(new_uri, new_route->second);
    return true;
  }
  return false;
}

// cgi client redirect response into http response
bool Cgi::makeClientRedirResponse_(const std::string& headers)
{
  Response res;
  res.setMessage(302);
  res.addHeader("Location", findHeaderValue_(headers, "Location: "));
  if (conn_.getOutBuffer().empty()) {
    res.addHeader("Content-Length: 0\r\n");
  }
  bufferResponse_(res);
  return true;
}

void Cgi::bufferResponse_(const Response& res)
{
  conn_.getOutBuffer().prepend("\r\n");
  for (const auto& header : res.getHeaders()) {
    conn_.getOutBuffer().prepend(header);
  }
  conn_.getOutBuffer().prepend(res.getMessage());
}
