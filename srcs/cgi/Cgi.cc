#include <unistd.h>
#include <sys/wait.h>

#include <array>

#include "Cgi.h"
#include "config/ConfigServer.h"
#include "io/task/SimpleBody.h"
#include "util/WebServ.h"

std::string Cgi::findHeaderValue_(const std::string& msg, const std::string& key)
{
  size_t start = msg.find(key) + key.length();
  size_t i = 0;
  while (!std::isspace(msg[start + i])) {
    i++;
  }
  return msg.substr(start, i);
}

void Cgi::delArr_(char** arr)
{
  size_t i = 0;
  if (!arr)
    return;
  while (arr[i]) {
    delete[] arr[i];
    i++;
  }
  delete[] arr;
}

// make environment variables as specified in CGI RFC
// some of them are missing due to not being required by subject
char** Cgi::makeEnv_(const Request& req)
{
  static struct header_null_helper {
    std::string operator()(const Request& req, const std::string& key) {
      const char* header = req.getHeader(key);
      if (!header) {
        return ("");
      }
      std::string str(header);
      size_t start = str.find(key) + key.length();
      size_t i = 0;
      while (!std::isspace(str[start + i])) {
        i++;
      }
      return str.substr(start, i);
    };
  } head;
  char** env = nullptr;
  std::array<std::string, 16> arr = {
    std::string("CONTENT_LENGTH=") + head(req, "Content-Length: "),
    std::string("CONTENT_TYPE=") + head(req, "Content-Type: "),
    std::string("GATEWAY_INTERFACE=CGI/1.1"),
    std::string("PATH_INFO=") + path_.substr(script_.length()),
    std::string("PATH_TRANSLATED=") + script_,
    std::string("QUERY_STRING=") +
      ((req.getUri().find("?") != std::string::npos) ? req.getUri().substr(req.getUri().find("?") + 1) : ""),
    std::string("REMOTE_ADDR=127.0.0.1"), // for now just hardcode localhost, ask lucas to pass the real thing
    std::string("REMOTE_HOST=") + head(req, "Host: "),
    std::string("REQUEST_METHOD=") + (req.getMethod() == HTTP::GET ? "GET" : "POST"),
    std::string("SCRIPT_NAME=") + script_,
    std::string("SERVER_NAME=") + head(req, "Host: "),
    std::string("SERVER_PORT=") + std::to_string(rh_.getConfigServer().getPort()),
    std::string("SERVER_PROTOCOL=HTTP/1.1"),
    std::string("SERVER_SOFTWARE=SuperWebserv10K/0.9.1 (Unix)")
  };
  env = new char*[arr.size() + 1];
  env[arr.size()] = nullptr;
  for (size_t i = 0; i < arr.size(); i++) {
    env[i] = new char[arr[i].length() + 1];
    arr[i].copy(env[i], arr[i].length());
    env[i][arr[i].length()] = '\0';
  }
  return (env);
}

std::string Cgi::getScriptName_(const std::string& path)
{
  return (path.substr(0, path.find(".cgi") + 4));
}

Cgi::Cgi(RequestHandler& rh, const std::string& path) : rh_(rh), path_(path), script_(getScriptName_(path)) {}

Cgi::~Cgi()
{
  delArr_(this->envp_);
}

void Cgi::act()
{
  envp_ = makeEnv_(rh_.getRequest());
  Response res;
  std::string result = execute_();
  std::string headers = result.substr(0, util::find_header_end(result));
  // document response
  if (headers.find("Content-Type") != std::string::npos) {
    makeDocumentResponse_(result, res);
  }
  // client-redir response (possibly with document)
  else if (headers.find("Location") != std::string::npos && headers.find("http://") != std::string::npos) {
    makeClientRedirResponse_(result, res);
  }
  // local-redir response AKA server redirect
  else if (headers.find("Location") != std::string::npos) {
    makeLocalRedirResponse_(result, res, rh_.getRequest());
  }
  // invalid response (not compliant with CGI spec)
  else {
    rh_.handleError_(500);
  }
}

// read from parent process on stdin
// execute script
// write output of script back to parent
// make sure to test for leaking fd's!!!
void Cgi::exec_child_()
{
  // if (rh_.getRequest().getContentLength() > 0) {
  //   dup2(this->pipe_in_[0], STDIN_FILENO);
  //   close(this->pipe_in_[0]);
  //   close(this->pipe_in_[1]);
  // }
  dup2(this->pipe_out_[1], STDOUT_FILENO);
  close(this->pipe_out_[0]);
  close(this->pipe_out_[1]);
  char* argv[] = {nullptr};
  execve(script_.c_str(), argv, envp_);
  // if we get here execve failed
  Log::error("executing CGI `", this->path_, "' failed: ", strerror(errno), '\n');
  exit(1);
}

// write request body to child's stdin
// wait for child then read child's stdout
// these read/write ops should go through epoll?!
std::string Cgi::exec_parent_(int pid)
{
  // if (rh_.getRequest().getContentLength() > 0) {
  //   close(this->pipe_in_[0]);
  //   if (write(this->pipe_in_[1], this->body_.c_str(), this->body_.length()) == -1) { // this won't work for cte
  //     return ("");
  //   }
  //   close(this->pipe_in_[1]);
  // }
  close(this->pipe_out_[1]);
  waitpid(pid, nullptr, 0);

  std::stringstream body;
  const ssize_t buf_size = 4096; // what would be optimal here?
  char buf[buf_size];
  ssize_t read_bytes;
  do {
    read_bytes = read(this->pipe_out_[0], buf, buf_size);
    if (read_bytes < 0)
      return(""); // TODO close fd's
    body.write(buf, read_bytes);
  } while (read_bytes == buf_size);

  close(this->pipe_out_[0]);
  return (body.str());
}

std::string Cgi::execute_()
{
  // open pipes, input pipe is only necessary if there is a body to write
  // both stdin are redirected the the other process stdout
  // if (this->body_.length() > 0) {
  //   if (pipe(this->pipe_in_) == -1) {
  //     return ("");
  //   }
  // }
  if (pipe(this->pipe_out_) == -1) {
    return ("");
  }

  // fork and run the parent and child process in separate functions
  int pid = fork();
  if (pid < 0) {
    return ("");
  }
  if (pid == 0) {
    exec_child_();
  }
  std::string result = exec_parent_(pid);
  return (result);
}

// function to process raw cgi document response into http response
void Cgi::makeDocumentResponse_(const std::string& raw, Response& res)
{
  size_t body_begin = util::find_header_end(raw);
  // size_t body_begin = raw.find("\n\n");
  if (body_begin == std::string::npos) { // not compliant with rfc
    rh_.handleError_(500);
    return;
  }
  body_begin += 2;
  auto dup = std::make_unique<char[]>(raw.length() - body_begin);
  raw.copy(dup.get(), raw.length(), body_begin);
  size_t substr_start = raw.find("Content-Type: ") + std::string("Content-Type: ").length();
  size_t substr_len = raw.find(';', substr_start) - substr_start; // spec does not specify ";" as delimiter?
  std::string content_type = raw.substr(substr_start, substr_len);
  res.addHeader("Server", "SuperWebserv10K/0.9.1 (Unix)");
  res.addHeader("Content-Length", std::to_string(raw.length() - body_begin));
  res.addHeader("Content-Type", content_type);
  res.setMessage(200);

  rh_.getConnection().enqueueResponse(std::move(res));
  rh_.getConnection().addTask(std::make_unique<SimpleBody>(std::move(dup), raw.length() - body_begin));
}

// function to process raw cgi local redirect response into http response
// this function has not been tested at all!!!
void Cgi::makeLocalRedirResponse_(const std::string& raw, Response& res, Request& req)
{
  (void) res;
  RequestHandler rh(rh_.getConnection(), rh_.getConfigServer(), req);
  std::string new_uri = findHeaderValue_(raw, "Location: ");
  auto new_route = rh_.getConfigServer().matchRoute(new_uri);
  if (new_route != rh_.getConfigServer().getRoutes().end()) {
    req.setUri(new_uri);
    rh.execRequest(new_uri, new_route->second);
    return;
  }
  rh.handleError_(500);
}

// function to process raw cgi client redirect response into http response
void Cgi::makeClientRedirResponse_(const std::string& raw, Response& res)
{
  // This is done to add the 302 body!
  res.setMessage(302);
  res.addHeader("Location", findHeaderValue_(raw, "Location: "));
  res.addHeader("Server", "SuperWebserv10K/0.9.1 (Unix)");
  // does not contain body
  if (raw.find("Content-Type") == std::string::npos) {
    return;
  }
  // contains body
  res.addHeader("Content-Type", findHeaderValue_(raw, "Content-Type: "));
  const std::string body = raw.substr(util::find_header_end(raw) + 2);
  res.addHeader("Content-Length", std::to_string(body.length()));
  auto dup = std::make_unique<char[]>(body.length());
  body.copy(dup.get(), body.length());
  rh_.getConnection().enqueueResponse(std::move(res));
  rh_.getConnection().addTask(std::make_unique<SimpleBody>(std::move(dup), body.length()));
}
