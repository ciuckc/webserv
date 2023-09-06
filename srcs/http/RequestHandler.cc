#include <sstream>
#include <dirent.h>
#include <fcntl.h>
#include "RequestHandler.h"
//#include "cgi/Cgi.h"
#include "util/WebServ.h"
#include "io/task/SendFile.h"
#include "io/task/SimpleBody.h"
#include "Status.h"
#include "io/task/DiscardBody.h"
#include "ErrorPage.h"
#include "MIME.h"

void  RequestHandler::execRequest(const ConfigRoute& route)
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
  if (!legalMethod_(route)) {
    handleError_(405);
  } else if (route.getRedir().length() != 0) {
    handleError_(302); // not an actual error but hey
    // get location from config and add header
  } else {
    // in case above functions get called without rooting the path, do it here
    // also, either update getPath to trim the pathinfo in case of cgi or just yeet it
    // because subject doesn't care about it anyways
    std::string path = request_.getPath();
    util::prepend_cwd(path);
    auto s = util::FileInfo();
    if (!s.open(path.c_str())) {
      handleError_(404);
    } else if (s.isDir()) {
      handleDir_(path, route, s);
    } else if (s.isFile()) {
      handleFile_(s, path);
    }
  }
  if (connection_.keepAlive())
    connection_.awaitRequest();
}

bool RequestHandler::legalMethod_(const ConfigRoute& route) const
{
  if (request_.getMethod() == HTTP::POST && request_.getUri().find(".cgi")) { // not %100 waterproof
    return (false);
  }
  return (route.isMethodAllowed(request_.getMethod()));
}

void RequestHandler::handleDir_(std::string& path, const ConfigRoute& route, FileInfo& file_info)
{
  for (const auto& file : route.getIndexFiles()) {
    if (!access(std::string(path + file).c_str(), R_OK)) {
      handleFile_(file_info, path + file);
      return;
    }
  }
  // if we get here it's autoindex or error
  if (route.isAutoIndex()){
    autoIndex_(path);
  }
  else {
    handleError_(405);
  }
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
  connection_.enqueueResponse(std::forward<Response>(Response::builder()
                                                         .message(200)
                                                         .content_length(body_size)
                                                         .header("Content-Type", "text/html")
                                                         .build()));
  connection_.addTask(std::make_unique<SimpleBody>(std::move(dup), body_size));
  if (request_.getContentLength() != 0)
    connection_.addTask(std::make_unique<DiscardBody>(request_.getContentLength()));
}

void RequestHandler::handleFile_(FileInfo& file_info, const std::string& path, int status, std::string type)
{
  // const std::string cgi_ext = ".cgi"; // fetch this from config instead
  // if (path.find(cgi_ext) != std::string::npos) {
  //   Cgi cgi(request_);
  //   response_ = cgi.act();
  //   return;
  // }
  bool addType = true;
  if (type.empty()) {
    std::string extension = util::getExtension(path);
    if (extension.empty() || (type = MIME.getType(extension)).empty())
      addType = false;
  }

  int fd = open(path.c_str(), O_RDONLY);
  if (fd == -1) { // todo: handle as error response
    throw IOException("shit's fucked yo", errno);
  }

  auto builder = Response::builder();
  builder.message(status)
         .content_length(file_info.size());
  if (addType)
    builder.header("Content-Type", type);
  connection_.enqueueResponse(std::forward<Response>(builder.build()));
  connection_.addTask(std::make_unique<SendFile>(fd));
}

void RequestHandler::handleError_(int error) {
  auto& error_pages = cfg_.getErrorPages();
  auto it = error_pages.find(error);
  if (it != error_pages.end()) {
    FileInfo s;
    if (s.open(it->second.c_str()) && s.isFile()) {
      handleFile_(s, it->second, error, "text/html");
      if (request_.getContentLength() != 0)
        connection_.addTask(std::make_unique<DiscardBody>(request_.getContentLength()));
      return;
    }
  }

  auto perr = http::defaultErrPage(error);
  size_t content_len = perr.first.getContentLength();
  connection_.enqueueResponse(std::move(perr.first));
  connection_.addTask(std::make_unique<SimpleBody>(std::move(perr.second), content_len));
  if (request_.getContentLength() != 0)
    connection_.addTask(std::make_unique<DiscardBody>(request_.getContentLength()));
}
