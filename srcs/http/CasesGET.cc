#include <sys/stat.h>
#include <exception>
#include <iostream>
#include <fstream>
#include "Cases.h"
#include "cgi/Cgi.h"
#include "util/WebServ.h"
#include "ErrorResponse.h"

using namespace get;
using namespace HTTP;

using stat_s = struct stat;

// maybe make this a member of response class?
// function to read file into body of response passed as param
size_t makeBody(Response& res, const char* type, const std::string& path)
{
  std::ios_base::openmode openmode = std::ios::in;
  if (type) {
    std::string type_str(type);
    if (type_str.find("text") != std::string::npos)
      openmode |= std::ios::binary;
  }
  std::ifstream file(path, openmode);
  if (!file.is_open()) {
    throw (ErrorResponse(500));
  }
  std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  file.close();
  size_t body_size = str.length();
  char* body;
  body = new char[body_size + 1];
  str.copy(body, body_size);
  body[body_size] = '\0';
  res.setBody(body, body_size);
  return (body_size);
}

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
  throw (ErrorResponse(404));
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
  size_t body_size = makeBody(res, type, path);
  res.addHeader("Server", "SuperWebserv10K/0.9.1 (Unix)");
  res.addHeader("Content-Length", std::to_string(body_size));
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
    size_t body_size = makeBody(res, type, path);
    res.addHeader("Server", "SuperWebserv10K/0.9.1 (Unix)");
    res.addHeader("Content-Length", std::to_string(body_size));
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
  this->cases_.push_back(std::make_unique<CaseFile>(CaseFile()));
  this->cases_.push_back(std::make_unique<CaseDir>());
  this->cases_.push_back(std::make_unique<CaseFail>());
}
