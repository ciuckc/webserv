#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <exception>
#include <iostream>
#include <fstream>
#include "Cases.h"
#include "cgi/Cgi.h"
#include "util/WebServ.h"
#include "ErrorResponse.h"

using namespace get;
using namespace HTTP;

// maybe make this a member of response class?
// function to read file into body of response passed as param
size_t makeBody(Response& res, const char* type, const std::string& path)
{
  uint8_t openmode = 0;
  if (type == NULL) {
    openmode &= std::ios::in; 
  }
  else {
    std::string type_str(type);
    openmode &= type_str.find("text") == std::string::npos ? std::ios::in : std::ios::binary;
  }
  std::ifstream file(path, openmode);
  if (!file.is_open()) {
    throw (ErrorResponse(500));
  }
  std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  file.close();
  size_t body_size = str.length();
  char* body;
  try {
    body = new char[body_size + 1];
  }
  catch (std::exception&) {
    throw (ErrorResponse(500));
  }
  str.copy(body, body_size);
  body[body_size] = '\0';
  res.setBody(body, body_size);
  return (body_size);
}

bool  CaseRedirect::test(Request& req) const
{
  // need config to check this
  (void) req;
  return (false);
}

Response  CaseRedirect::act(Request& req) const
{
  (void) req;
  return (Response());
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
  struct stat s;
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
    throw (ErrorResponse(500));
  }
  return (res);
}

bool  CaseNoFile::test(Request& req) const
{
  struct stat s;
  std::string path = req.getPath();
  prepend_cwd(path);
  return (stat(path.c_str(), &s));
}

Response  CaseNoFile::act(Request& req) const
{
  (void) req;
  throw (ErrorResponse(404));
}

bool  CaseFile::test(Request& req) const
{
  struct stat s;
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
  struct stat s;
  std::string path = req.getPath();
  prepend_cwd(path);
  stat(path.c_str(), &s);
  return (s.st_mode & S_IFDIR);
}

Response  CaseDir::act(Request& req) const
{
  Response res;
  struct stat s;
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
bool  CaseFail::test(Request& req) const
{
  (void) req;
  return (true);
}

Response  CaseFail::act(Request& req) const
{
  (void) req;
  std::runtime_error("400 invalid request");
  return (Response());
}

// make it static or some singleton shit
CasesGET::CasesGET()
{
  try
  {
    this->cases_.push_back(new CaseRedirect());
    this->cases_.push_back(new CaseCGI());
    this->cases_.push_back(new CaseNoFile());
    this->cases_.push_back(new CaseFile());
    this->cases_.push_back(new CaseDir());
    this->cases_.push_back(new CaseFail());
  }
  catch (std::exception&)
  {
     throw (ErrorResponse(500));
  }
}

CasesGET::~CasesGET()
{
  while (!this->cases_.empty())
  {
    delete (this->cases_.back());
    this->cases_.pop_back();
  }
}
