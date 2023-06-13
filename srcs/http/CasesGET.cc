#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <exception>
#include <iostream>
#include "Cases.h"

using namespace get;

static void st_prepend_cwd(std::string& str)
{
  char* cwd;
  cwd = getcwd(NULL, 0);
  if (cwd == NULL)
  {
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
  if (path.length() < 4)
  {
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
  // std::cout << "proper handler called" << std::endl;
  // plop the file in a response
  (void) req;

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
  std::runtime_error("500 internal error");
  return (Response());
}

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
