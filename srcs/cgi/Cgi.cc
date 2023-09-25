#include <unistd.h>
#include <sys/wait.h>

#include <array>

#include "Cgi.h"
#include "http/RequestHandler.h"
#include "config/ConfigServer.h"
#include "io/task/SimpleBody.h"
#include "util/WebServ.h"

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
char** Cgi::makeEnv(const Request& req, const std::string& path, const ConfigServer& cfg)
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
  char** env = nullptr;
  std::string script = getScriptName(path);
  std::array<std::string, 16> arr = {
    std::string("CONTENT_LENGTH=") + head(req, "Content-Length: "),
    std::string("CONTENT_TYPE=") + head(req, "Content-Type: "),
    std::string("GATEWAY_INTERFACE=CGI/1.1"),
    std::string("PATH_INFO=") + path.substr(script.length()),
    std::string("PATH_TRANSLATED=") + script,
    std::string("QUERY_STRING=") +
      ((req.getUri().find("?") != std::string::npos) ? req.getUri().substr(req.getUri().find("?") + 1) : ""),
    std::string("REMOTE_ADDR=127.0.0.1"), // for now just hardcode localhost, ask lucas to pass the real thing
    std::string("REMOTE_HOST=") + head(req, "Host: "),
    std::string("REQUEST_METHOD=") + (req.getMethod() == HTTP::GET ? "GET" : "POST"),
    std::string("SCRIPT_NAME=") + script,
    std::string("SERVER_NAME=") + head(req, "Host: "),
    std::string("SERVER_PORT=") + std::to_string(cfg.getPort()),
    std::string("SERVER_PROTOCOL=HTTP/1.1"),
    std::string("SERVER_SOFTWARE=SuperWebserv10K/0.9.1 (Unix)")
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

Cgi::Cgi(const ConfigServer& config_server, Connection& conn, const std::string& path)
  : cfg_(config_server),
    conn_(conn),
    path_(path) {}

void Cgi::act(const std::string& headers)
{
  // document response
  if (headers.find("Content-Type") != std::string::npos) {
    makeDocumentResponse_(headers);
  }
  // client-redir response
  else if (headers.find("Location") != std::string::npos && headers.find("http://") != std::string::npos) {
    makeClientRedirResponse_(headers);
  }
  // local-redir response AKA server redirect
  else if (headers.find("Location") != std::string::npos) {
    makeLocalRedirResponse_(headers);
  }
  // invalid response (not compliant with CGI spec)
  else {
    // rh_.handleError_(500);
  }
}

// cgi document response into http response
void Cgi::makeDocumentResponse_(const std::string& headers)
{
  Response res;
  res.addHeader("Transfer-Encoding", "Chunked");
  res.addHeader("Content-Type", findHeaderValue_(headers, "Content-Type: "));
  res.setMessage(200);
  // conn_.enqueueResponse(std::move(res));
  bufferResponse_(res);
}

// cgi local redirect response into http response
void Cgi::makeLocalRedirResponse_(const std::string& headers)
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
    return;
  }
  // rh.handleError_(500);
}

// cgi client redirect response into http response
void Cgi::makeClientRedirResponse_(const std::string& headers)
{
  Response res;
  res.setMessage(302);
  res.addHeader("Location", findHeaderValue_(headers, "Location: "));
  conn_.enqueueResponse(std::move(res));
}

void Cgi::bufferResponse_(const Response& res)
{
  (void) res;
  conn_.getOutBuffer().prepend("\r\n");
  for (const auto& header : res.getHeaders()) {
    conn_.getOutBuffer().prepend(header);
  }
  conn_.getOutBuffer().prepend(res.getMessage());
}
