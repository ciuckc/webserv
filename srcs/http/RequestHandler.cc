#include <sstream>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include "RequestHandler.h"
#include "ErrorResponse.h"
#include "cgi/Cgi.h"
#include "util/WebServ.h"

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
    handleDir_(path);
    return;
  }
  if (s.st_mode & S_IFREG) {
    handleFile_(path);
    return;
  }
}

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

void RequestHandler::handleDir_(std::string& path)
{
  // get array of index files from config
  // for each: if (!access(that file)):
  // response_ = handleFile_(that file)

  // if we get here it's autoindex or error
  // for now assume autoindex is enabled
  autoIndex_(path);
}

void RequestHandler::autoIndex_(std::string& path)
{
  std::ostringstream body;
  std::string name;
  DIR* dir = opendir(path.c_str());
  if (dir == nullptr) {
    response_ = ErrorResponse(500);
    return;
  }
  struct dirent* entry = readdir(dir);
  body << "<html>" << '\n' << "<body>" << '\n';
  while (entry != nullptr) {
    name = entry->d_name;
    if (entry->d_type == DT_DIR) {
      name += "/";
    }
    body << "<a href=\"" << request_.getUri() << name << "\">" << name << "</a>" << "<br />" << '\n';
    entry = readdir(dir);
  }
  closedir(dir);
  body << "</body>" << '\n' << "</html>" << '\n';
  size_t body_size = body.str().length();
  char* dup;
  try {
    dup = new char[body_size + 1];
  }
  catch (std::exception&) {
    response_ = ErrorResponse(500);
    return;
  }
  body.str().copy(dup, body_size);
  dup[body_size] = '\0';
  response_.setBody(dup, body_size);
  response_.addHeader("Content-Type", "text/html");
  response_.addHeader("Content-Length", std::to_string(body_size));
  response_.setMessage(200);
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

Response&& RequestHandler::getResponse()
{
  return std::move(this->response_);
}
