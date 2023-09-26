#include "RequestHandler.h"

#include <dirent.h>
#include <fcntl.h>

#include <sstream>

#include "ErrorPage.h"
#include "MIME.h"
#include "Status.h"
#include "cgi/Cgi.h"
#include "config/ConfigServer.h"
#include "util/WebServ.h"
#include "io/task/SendFile.h"
#include "io/task/SimpleBody.h"
#include "io/task/DiscardBody.h"
#include "io/task/SpliceOut.h"
#include "io/task/SpliceIn.h"

void  RequestHandler::execRequest(const std::string& path, const ConfigRoute& route)
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

  if (!legalMethod_(route)) {
    handleError_(405);
  } else if (route.getRedir().length() != 0) {
    handleRedir_(route);
  } else {
    // in case above functions get called without rooting the path, do it here
    auto s = util::FileInfo();
    std::string cgi_no_pathinfo;
    if (path.find(".cgi") != std::string::npos) { // get ext from config instead
      cgi_no_pathinfo = path.substr(0, path.find(".cgi") + 4);
    }
    if (!s.open(path.c_str()) && cgi_no_pathinfo.empty()) {
      handleError_(404);
    } else if (s.isDir()) {
      handleDir_(path, route, s);
    } else if (s.isFile() || !cgi_no_pathinfo.empty()) {
      handleFile_(s, path);
    }
  }
}

bool RequestHandler::legalMethod_(const ConfigRoute& route) const
{
  if (request_.getMethod() == HTTP::POST && request_.getUri().find(".cgi") == std::string::npos) { // not %100 waterproof
    return (false);
  }
  return (route.isMethodAllowed(request_.getMethod()));
}

void RequestHandler::handleDir_(const std::string& path, const ConfigRoute& route, FileInfo& file_info) {
  if (request_.getMethod() == HTTP::DELETE) {
    return handleError_(400);
  }
  std::string actual_path = (*path.rbegin() == '/') ? path : path + '/';
  for (const auto& file : route.getIndexFiles()) {
    std::string actual_file = actual_path + file;
    if (!access(actual_file.c_str(), R_OK)) {
      file_info.open(actual_file.data());
      handleFile_(file_info, actual_file);
      return;
    }
  }
  if (route.isAutoIndex()) {
    autoIndex_(path);
  } else {
    handleError_(403);
  }
}

void RequestHandler::autoIndex_(const std::string& path)
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
                                                         .header("Content-Type: text/html\r\n")
                                                         .build()));
  connection_.addTask(std::make_unique<SimpleBody>(std::move(dup), body_size));
  if (request_.getContentLength() != 0)
    connection_.addTask(std::make_unique<DiscardBody>(request_.getContentLength()));
}

void RequestHandler::deleteFile_(const std::string& path)
{
  if (unlink(path.c_str())) {
    return (handleError_(500));
  }
  auto builder = Response::builder();
  builder.message(204);
  connection_.enqueueResponse(std::forward<Response>(builder.build()));
}

void RequestHandler::handleFile_(FileInfo& file_info, const std::string& path)
{
  if (request_.getMethod() == HTTP::DELETE) {
    return (deleteFile_(path));
  }
  const std::string cgi_ext = ".cgi"; // fetch this from config instead
  if (path.find(cgi_ext) != std::string::npos) {
    return (handleCgi_(path));
  }
  bool addType = true;
  std::string extension = util::getExtension(path);
  std::string type;
  if (extension.empty() || (type = MIME.getType(extension)).empty())
    addType = false;

  int fd = open(path.c_str(), O_RDONLY);
  if (fd == -1) {
    switch (errno) {
      case EACCES:
      case EISDIR:
        handleError_(403);
        return;
      case ENOENT:
        handleError_(404);
        return;
      default:
        handleError_(500);
        return;
    }
  }

  auto builder = Response::builder();
  builder.message(200)
         .content_length(file_info.size());
  if (addType)
    builder.header("Content-Type", type);
  connection_.enqueueResponse(std::forward<Response>(builder.build()));
  connection_.addTask(std::make_unique<SendFile>(fd, file_info.size()));
  Log::trace(connection_, "Adding SendFile(", path, ") to queue\n");
}

void RequestHandler::handleCgi_(const std::string& path)
{
  int pipe_in[2];
  int pipe_out[2];
  if (pipe(pipe_out) == -1 || pipe(pipe_in) == -1) {
    return (handleError_(500));
  }
  size_t len = request_.getContentLength();
  if (len != 0) {
    Log::trace("Reading request body of length: ", len, " into pipe\n");
    connection_.addTask(std::make_unique<SpliceIn>(connection_.getServer(), connection_, pipe_in[1], len));
  }
  connection_.addTask(std::make_unique<SpliceOut>(connection_.getServer(), connection_, path, cfg_, pipe_out[0]));
  char** envp = Cgi::makeEnv(request_, path, cfg_);
  int pid = fork();
  if (pid < 0) {
    return (handleError_(500));
  }
  if (pid == 0) { // child
    dup2(pipe_in[0], STDIN_FILENO);
    dup2(pipe_out[1], STDOUT_FILENO);
    close(pipe_in[0]);
    close(pipe_in[1]);
    close(pipe_out[0]);
    close(pipe_out[1]);
    char** argv = {nullptr};
    execve(Cgi::getScriptName(path).c_str(), argv, envp);
    // if we get here execve failed
    Log::error("executing CGI failed: ", strerror(errno), '\n');
    exit(1);
  }
  // parent
  close(pipe_in[0]);
  close(pipe_out[1]);
  Cgi::delEnv(envp);
  Log::trace("Cgi setup done\n");
}

void RequestHandler::handleError_(int error) {
  auto perr = http::createError(cfg_, error);
  connection_.enqueueResponse(std::move(perr.first));
  connection_.addTask(std::move(perr.second));
  if (request_.getContentLength() != 0)
    connection_.addTask(std::make_unique<DiscardBody>(request_.getContentLength()));
}

void RequestHandler::handleRedir_(const ConfigRoute& route) {
  auto perr = http::createError(cfg_, 302);
  perr.first.addHeader("Location", route.getRedir());
  connection_.enqueueResponse(std::move(perr.first));
  connection_.addTask(std::move(perr.second));
  if (request_.getContentLength() != 0)
    connection_.addTask(std::make_unique<DiscardBody>(request_.getContentLength()));
}
