#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <exception>
#include "Cases.h"

using namespace get;

bool  CaseRedirect::test(Request& req) const
{
  // need config to check this
  return (false);
}

void  CaseRedirect::act(Request& req) const
{
}

bool  CaseNoFile::test(Request& req) const
{
  struct stat s;
  return (stat(req.GetUri().c_str(), &s));
}

void  CaseNoFile::act(Request& req) const
{
  std::runtime_error("404 file not found");
}

bool  CaseCGI::test(Request& req) const
{
  // need config for this
  return (false);
}

void  CaseCGI::act(Request& req) const
{
}

bool  CaseFile::test(Request& req) const
{
  struct stat s;
  stat(req.GetUri().c_str(), &s);
  return (s.st_mode & S_IFREG);
}

void  CaseFile::act(Request& req) const
{
  // plop the file in a response
}

bool  CaseDir::test(Request& req) const
{
  struct stat s;
  stat(req.GetUri().c_str(), &s);
  return (s.st_mode & S_IFDIR);
}

void  CaseDir::act(Request& req) const
{
  std::string path = "index.html"; // or whatever is set in config
  path.insert(0, req.GetUri());
  struct stat s;
  if (stat(req.GetUri().c_str(), &s))
  {
    // no file, list directory if enabled in config
  }
  else
  {
    // put index in response
  }
}

// if we hit this case it must be a failure
bool  CaseFail::test(Request& req) const
{
  return (true);
}

void  CaseFail::act(Request& req) const
{
  std::runtime_error("500 internal error");
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
