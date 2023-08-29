#include <unistd.h>
#include <sys/stat.h>
#include "RequestHandler.h"
#include "ErrorResponse.h"
#include "cgi/Cgi.h"
#include "util/WebServ.h"

bool RequestHandler::legalMethod_() const
{
  // here we want to check config to see allowed methods for this route
  // if method == POST, also check if path contains the cgi extension
  // after finding the right route/location save it for later reference
  //
  // should the root be applied before checking tho??
  return (true);
}

bool RequestHandler::isRedirect_() const
{
  // check config to see if a redirect is defined for this route
  //
  // should the root be applied before checking tho??
  return (false);
}

void RequestHandler::handleDir_()
{
  // get array of index files from config
  // for each: if (!access(that file)):
  // return handleFile_(that file)

  // if we get here it's autoindex or error
  // for now assume autoindex is enabled
  // response_ = autoindex_();
  response_ = ErrorResponse(200);
}

void RequestHandler::handleFile_(std::string& path)
{
  const std::string cgi_ext = ".cgi"; // fetch this from config instead
  if (path.find(cgi_ext) != std::string::npos) {
    Cgi cgi(request_);
    response_ = cgi.act();
    return;
  }
  const char* type = request_.getHeader("Content-Type");
  response_.makeBody(type, path);
  response_.addHeader("Content-Length", std::to_string(response_.getBodySize()));
  if (type) {
    response_.addHeader("Content-Type", std::string(type));
  }
  response_.setMessage(200);
}

void  RequestHandler::execRequest()
{
  // find correct configserver or return 400
  if (!legalMethod_()) {
    response_ = ErrorResponse(405);
    return;
  }
  if (isRedirect_()) {
    response_ = ErrorResponse(302); // not an actual error but hey
    // get location from config and add header
    return;
  }
  // in case above functions get called without rooting the path, do it here
  // also, either update getPath to trim the pathinfo in case of cgi or just yeet it 
  // because subject doesn't care about it anyways
  std::string path = request_.getPath();
  HTTP::prepend_cwd(path);
  struct stat s;
  if (stat(path.c_str(), &s)) {
    response_ = ErrorResponse(404);
    return;
  }
  if (s.st_mode & S_IFDIR) {
    handleDir_();
    return;
  }
  if (s.st_mode & S_IFREG) {
    handleFile_(path);
    return;
  }
}

Response&& RequestHandler::getResponse()
{
  return std::move(this->response_);
}
