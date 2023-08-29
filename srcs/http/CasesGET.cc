#include <sys/stat.h>
#include <exception>
#include <iostream>
#include <fstream>
#include "Cases.h"
#include "cgi/Cgi.h"
#include "util/WebServ.h"
#include "ErrorResponse.h"

using namespace get;
using namespace util;

using stat_s = struct stat;

bool  CaseRedirect::test(Request&) const
{
  return (false);
}

Response  CaseRedirect::act(Request&) const
{
  return {};
}

bool  CaseCGI::test(Request& req) const
{
  // need config for this
  // for now just check if it ends in .cgi
  const std::string cgi_ext = ".cgi";
  std::string path = req.getPath();
  prepend_cwd(path);
  size_t path_end = path.find(cgi_ext);
  if (path_end == std::string::npos) {
    return (false);
  }
  path_end += cgi_ext.length();
  path = path.substr(0, path_end);
  stat_s s;
  stat(path.c_str(), &s);
  return (s.st_mode & S_IFREG);
}

Response  CaseCGI::act(Request& req) const
{
  Response res;
  Cgi cgi(req);
  std::string result = cgi.execute();
  std::string headers = result.substr(0, find_header_end(result));
  // document response
  if (headers.find("Content-Type") != std::string::npos) {
    Cgi::makeDocumentResponse(result, res);
  }
  // client-redir response (possibly with document)
  else if (headers.find("Location") != std::string::npos && headers.find("http://") != std::string::npos) {
    Cgi::makeClientRedirResponse(result, res);
  }
  // local-redir response AKA server redirect
  else if (headers.find("Location") != std::string::npos) {
    Cgi::makeLocalRedirResponse(result, res, req);
  }
  // invalid response (not compliant with CGI spec)
  else {
    return ErrorResponse(500);
  }
  return res;
}

bool  CaseNoFile::test(Request& req) const
{
  stat_s s;
  std::string path = req.getPath();
  prepend_cwd(path);
  return (stat(path.c_str(), &s));
}

Response  CaseNoFile::act(Request&) const
{
  return (ErrorResponse(404));
}

bool  CaseFile::test(Request& req) const
{
  stat_s s;
  std::string path = req.getPath();
  prepend_cwd(path);
  stat(path.c_str(), &s);
  return (s.st_mode & S_IFREG);
}

Response  CaseFile::act(Request& req) const
{
  Response res;
  std::string path = req.getPath();
  prepend_cwd(path);
  const char* type = req.getHeader("Content-Type");
  res.makeBody(type, path);
  res.addHeader("Server", "SuperWebserv10K/0.9.1 (Unix)");
  res.addHeader("Content-Length", std::to_string(res.getBodySize()));
  res.setMessage(200);
  return (res);
}

bool  CaseDir::test(Request& req) const
{
  stat_s s;
  std::string path = req.getPath();
  prepend_cwd(path);
  stat(path.c_str(), &s);
  return (s.st_mode & S_IFDIR);
}

Response  CaseDir::act(Request& req) const
{
  Response res;
  stat_s s;
  std::string path = req.getPath();
  prepend_cwd(path);
  path.append("index.html");     // or other file specified in config
  if (stat(path.c_str(), &s)) {  // no file, list directory if enabled in config
    return ErrorResponse(403);
  }
  else {                         // file exists, serve it
    const char* type = req.getHeader("Content-Type");
    res.makeBody(type, path);
    res.addHeader("Server", "SuperWebserv10K/0.9.1 (Unix)");
    res.addHeader("Content-Length", std::to_string(res.getBodySize()));
    res.setMessage(200);
  }
  return (res);
}

// if we hit this case it must be a failure
bool  CaseFail::test(Request&) const
{
  return (true);
}

Response  CaseFail::act(Request&) const
{
  return ErrorResponse(400);
}

CasesGET::CasesGET()
{
  this->cases_.push_back(std::make_unique<CaseRedirect>());
  this->cases_.push_back(std::make_unique<CaseCGI>());
  this->cases_.push_back(std::make_unique<CaseNoFile>());
  this->cases_.push_back(std::make_unique<CaseFile>());
  this->cases_.push_back(std::make_unique<CaseDir>());
  this->cases_.push_back(std::make_unique<CaseFail>());
}
