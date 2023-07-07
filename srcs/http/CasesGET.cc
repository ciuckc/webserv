#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <exception>
#include <iostream>
#include <fstream>
#include "Cases.h"
#include "ErrorResponse.h"

using namespace get;

static void st_prepend_cwd(std::string& str)
{
  char* cwd;
  cwd = getcwd(NULL, 0);
  if (cwd == NULL) {
    ErrorResponse(500);
  }
  str.insert(0, cwd);
  free(cwd);
}

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
    ErrorResponse(500);
  }
  std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  file.close();
  size_t body_size = str.length();
  char* body;
  try {
    body = new char[body_size];
  }
  catch (std::exception&) {
    ErrorResponse(500);
  }
  str.copy(body, body_size);
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

bool  CaseNoFile::test(Request& req) const
{
  struct stat s;
  std::string path = req.getPath();
  st_prepend_cwd(path);
  return (stat(path.c_str(), &s));
}

Response  CaseNoFile::act(Request& req) const
{
  (void) req;
  return (ErrorResponse(404));
}

bool  CaseCGI::test(Request& req) const
{
  // need config for this
  // for now just check if it ends in .py
  std::string path = req.getPath();
  st_prepend_cwd(path);
  if (path.length() < 4) {
    return (false);
  }
  return (!path.compare(path.length() - 3, 3, ".py")); // doesn't work with cgi params
}

Response  CaseCGI::act(Request& req) const
{
  (void) req;
  // fork and exec
  // read std out back into response
  return (Response());
}

bool  CaseFile::test(Request& req) const
{
  struct stat s;
  std::string path = req.getPath();
  st_prepend_cwd(path);
  stat(path.c_str(), &s);
  return (s.st_mode & S_IFREG);
}

Response  CaseFile::act(Request& req) const
{
  Response res;
  std::string path = req.getPath();
  st_prepend_cwd(path);
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
  st_prepend_cwd(path);
  stat(path.c_str(), &s);
  return (s.st_mode & S_IFDIR);
}

Response  CaseDir::act(Request& req) const
{
  Response res;
  struct stat s;
  std::string path = req.getPath();
  st_prepend_cwd(path);
  path.append("index.html");     // or other file specified in config
  if (stat(path.c_str(), &s)) {  // no file, list directory if enabled in config
    return (ErrorResponse(403));
  }
  else {                         // file exists, serve it
    std::cout << "enters" << std::endl;
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
    this->cases_.push_back(new CaseNoFile());
    this->cases_.push_back(new CaseCGI());
    this->cases_.push_back(new CaseFile());
    this->cases_.push_back(new CaseDir());
    this->cases_.push_back(new CaseFail());
  }
  catch (std::exception&)
  {
    ErrorResponse(500);
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
