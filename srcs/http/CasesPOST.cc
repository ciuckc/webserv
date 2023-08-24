#include <sys/stat.h>
#include "Cases.h"
#include "cgi/Cgi.h"
#include "util/WebServ.h"
#include "ErrorResponse.h"

using namespace HTTP;
using namespace post;

using stat_s = struct stat;

bool  CaseStaticContent::test(Request& req) const
{
  stat_s s;
  std::string path = req.getPath();
  prepend_cwd(path);
  stat(path.c_str(), &s);
  return (s.st_mode & S_IFDIR || s.st_mode & S_IFREG);
}

Response  CaseStaticContent::act(Request& req) const
{
  (void) req;
  Response res = ErrorResponse(405);
  return (res);
}

CasesPOST::CasesPOST()
{
  this->cases_.push_back(std::make_unique<CaseRedirect>());
  this->cases_.push_back(std::make_unique<CaseCGI>());
  this->cases_.push_back(std::make_unique<CaseNoFile>());
  this->cases_.push_back(std::make_unique<CaseStaticContent>());
  this->cases_.push_back(std::make_unique<CaseFail>());
}
