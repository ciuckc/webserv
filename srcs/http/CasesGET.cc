#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <exception>
#include <iostream>
#include <fstream>
#include "Cases.h"

using namespace get;

static void st_prepend_cwd(std::string& str)
{
  char* cwd;
  cwd = getcwd(NULL, 0);
  if (cwd == NULL) {
    std::runtime_error("500 internal error");
  }
  str.insert(0, cwd);
  free(cwd);
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
  std::string path = req.GetPath();
  st_prepend_cwd(path);
  return (stat(path.c_str(), &s));
}

Response  CaseNoFile::act(Request& req) const
{
  (void) req;
  std::runtime_error("404 file not found");
  return (Response());
}

bool  CaseCGI::test(Request& req) const
{
  // need config for this
  // for now just check if it ends in .py
  std::string path = req.GetPath();
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
  std::string path = req.GetPath();
  st_prepend_cwd(path);
  stat(path.c_str(), &s);
  return (s.st_mode & S_IFREG);
}

Response  CaseFile::act(Request& req) const
{
  // when it works move this to a generic function!
  Response res;
  uint8_t openmode = 0;
  try {
    std::vector<std::string> types = req.GetHeader("Content-Type")->values_;
    openmode &= types[0].find("text") == std::string::npos ? std::ios::in : std::ios::binary;
  }
  catch (std::exception&) {
    openmode &= std::ios::in; 
  }
  std::string path = req.GetPath();
  st_prepend_cwd(path);
  std::ifstream file(path, openmode); // get MIME type to set openmode
  if (!file.is_open()) {
    std::runtime_error("500 internal error");
  }
  std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  size_t body_size = str.length();
  char* body;
  try {
    body = new char[body_size];
  }
  catch (std::exception&) {
    std::runtime_error("500 internal error");
  }
  str.copy(body, body_size);
  res.setBody(body, body_size);
  // res.addHeader("Content-Type", 
  // res.addHeader("Content-Length", 
  // res.addHeader("Server", 
  res.setMessage(200);
  return (Response());
}

bool  CaseDir::test(Request& req) const
{
  struct stat s;
  std::string path = req.GetPath();
  st_prepend_cwd(path);
  stat(path.c_str(), &s);
  return (s.st_mode & S_IFDIR);
}

Response  CaseDir::act(Request& req) const
{
  struct stat s;
  std::string path = req.GetPath();
  st_prepend_cwd(path);
  path.append("index.html");   // or other file specified in config
  if (stat(path.c_str(), &s))
  {
    // no file, list directory if enabled in config
  }
  else
  {
    // put index in response
  }
  return (Response());
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

// not efficient to do this for each request, figure out something better!!
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
    std::runtime_error("500 internal error");
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
