#include <sstream>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "RequestHandler.h"
//#include "cgi/Cgi.h"
#include "util/WebServ.h"
#include "io/task/SendFile.h"
#include "io/task/SimpleBody.h"
#include "Status.h"
#include "io/task/DiscardBody.h"
#include "ErrorPage.h"

void  RequestHandler::execRequest()
{
  // for route in cfg_.routes
  //   if route matches (location, method, all that)
  //     add ITask to get body if necessary
  //     add OTask to send response
  //     add OTask to send body
  //     return

  // No route matches, route matching could set error code
  // (if a route is matched but incorrect method for example)
  // add DiscardBody OTask to eat body
  // add SendFile task for error page

  // find correct configserver or return 400
  if (!legalMethod_()) {
    handleError_(405);
  } else if (isRedirect_()) {
    handleError_(302); // not an actual error but hey
    // get location from config and add header
  } else {
    // in case above functions get called without rooting the path, do it here
    // also, either update getPath to trim the pathinfo in case of cgi or just yeet it
    // because subject doesn't care about it anyways
    std::string path = request_.getPath();
    util::prepend_cwd(path);
    stat_t s;
    if (stat(path.c_str(), &s)) {
      handleError_(404);
    } else if ((s.st_mode & S_IFMT) == S_IFDIR) {
      handleDir_(path);
    } else if ((s.st_mode & S_IFMT) == S_IFREG) {
      handleFile_(s, path);
    }
  }
  if (connection_.keepAlive())
    connection_.awaitRequest();
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
  if (dir == nullptr)
    return handleError_(500);

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
  auto dup = std::make_unique<char[]>(body_size + 1);
  body.str().copy(dup.get(), body_size);
  Response response;
  response.addHeader("Content-Type", "text/html");
  response.addHeader("Content-Length", std::to_string(body_size));
  response.setMessage(200);
  connection_.enqueueResponse(std::move(response));
  connection_.addTask(std::make_unique<SimpleBody>(std::move(dup), body_size));
  if (request_.getContentLength() != 0)
    connection_.addTask(std::make_unique<DiscardBody>(request_.getContentLength()));
}

void RequestHandler::handleFile_(stat_t& file_info, const std::string& path, int status)
{
 // const std::string cgi_ext = ".cgi"; // fetch this from config instead
 // if (path.find(cgi_ext) != std::string::npos) {
 //   Cgi cgi(request_);
 //   response_ = cgi.act();
 //   return;
 // }
  Response response;
  int fd = open(path.c_str(), O_RDONLY);
  if (fd == -1) { // todo: handle as error response
    throw IOException("shit's fucked yo", errno);
  }
  response.setMessage(status);
  response.setContentLength(file_info.st_size);
  response.addHeader("Content-Length", std::to_string(file_info.st_size));
  response.addHeader("Content-Type", request_.getContentType());
  connection_.enqueueResponse(std::move(response));
  connection_.addTask(std::make_unique<SendFile>(fd));
}

void RequestHandler::handleError_(int error) {
  auto& error_pages = cfg_.getErrorPages();
  auto it = error_pages.find(error);
  if (it != error_pages.end()) {
    stat_t s;
    if (stat(it->second.c_str(), &s)) {
      return handleFile_(s, it->second, error);
    }
  }

  auto perr = http::defaultErrPage(error);
  size_t content_len = perr.first.getContentLength();
  connection_.enqueueResponse(std::move(perr.first));
  connection_.addTask(std::make_unique<SimpleBody>(std::move(perr.second), content_len));
  if (request_.getContentLength() != 0)
    connection_.addTask(std::make_unique<DiscardBody>(request_.getContentLength()));
}
