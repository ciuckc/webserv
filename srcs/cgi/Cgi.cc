#include <array>
#include <unistd.h>
#include <iostream>
#include <sys/wait.h>
#include <sstream>
#include "Cgi.h"
#include "util/WebServ.h"
#include "http/RequestHandler.h"

static std::string st_find_header_value(const std::string& msg, const std::string& key)
{
  size_t start = msg.find(key) + key.length();
  size_t i = 0;
  while (!std::isspace(msg[start + i])) {
    i++;
  }
  return msg.substr(start, i);
}

static void st_del_arr(char** arr)
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
static char** st_make_env(const Request& req)
{
  static struct header_null_helper {
    std::string operator()(const Request& req, const std::string& key) {
      const char* header = req.getHeader(key);
      return { header ? header : "" };
    };
  } head;
  char** env = nullptr;
  const std::string script = req.getPath().substr(0, req.getPath().find(".cgi") + 4);
  std::array<std::string, 16> arr = {
    std::string("AUTH_TYPE="),
    std::string("CONTENT_LENGTH=") + head(req, "Content-Length"),
    std::string("CONTENT_TYPE=") + head(req, "Content-Type"),
    std::string("GATEWAY_INTERFACE=CGI/1.1"),
    std::string("PATH_INFO=") + script,
    std::string("PATH_TRANSLATED="), // root path_info based on confi
    std::string("QUERY_STRING=") + req.getUri().substr(req.getUri().find('?') + 1),
    std::string("REMOTE_ADDR=127.0.0.1"), // for now just hardcode localhost, ask lucas to pass the real thing
    std::string("REMOTE_HOST=") + 
      std::string(req.getHeader("Host") ? st_find_header_value(req.getHeader("Host"), "Host: ") : ""),
    std::string("REMOTE_USER="), // not sure that we need this as we're not doing authentication?
    std::string("REQUEST_METHOD=") + (req.getMethod() == HTTP::GET ? "GET" : "POST"),
    std::string("SCRIPT_NAME=") + script,
    std::string("SERVER_NAME=SuperWebserv10K/0.9.1 (Unix)"),
    std::string("SERVER_PORT=6969"),
    std::string("SERVER_PROTOCOL=HTTP/1.1"),
    std::string("SERVER_SOFTWARE=SuperWebserv10K/0.9.1 (Unix)")
  };
  try {
    env = new char*[arr.size() + 1];
    env[arr.size()] = nullptr;
    for (size_t i = 0; i < arr.size(); i++) {
      env[i] = new char[arr[i].length() + 1];
      arr[i].copy(env[i], arr[i].length());
      env[i][arr[i].length()] = '\0';
    }
  }
  catch (std::exception&) {
    st_del_arr(env);
    throw (ErrorResponse(500));
  }
  return (env);
}

Cgi::Cgi(const Request& req) :
    body_(req.getBody()),
    path_("." + req.getPath().substr(0, req.getPath().find(".cgi") + 4)), // fix getPath() !!! (or confirm that it's working)
    envp_(st_make_env(req)),
    req_(req)
{}

Cgi::~Cgi()
{
  st_del_arr(this->envp_);
}

Response  Cgi::act()
{
  Response res;
  std::string result = execute_();
  std::string headers = result.substr(0, HTTP::find_header_end(result));
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
    makeLocalRedirResponse_(result, res, req_);
  }
  // invalid response (not compliant with CGI spec)
  else {
    return ErrorResponse(500);
  }
  return res;
}

// read from parent process on stdin
// execute script
// write output of script back to parent
// make sure to test for leaking fd's!!!
void Cgi::exec_child_()
{
  if (this->body_.length() > 0) {
    dup2(this->pipe_in_[0], STDIN_FILENO);
    close(this->pipe_in_[0]);
    close(this->pipe_in_[1]);
  }
  dup2(this->pipe_out_[1], STDOUT_FILENO);
  close(this->pipe_out_[0]);
  close(this->pipe_out_[1]);
  char* argv[] = {nullptr};
  execve(this->path_.c_str(), argv, this->envp_);
  // if we get here execve failed
  Log::error("executing CGI `", this->path_, "' failed: ", strerror(errno), '\n');
  exit(1);
}

// write request body to child's stdin
// wait for child then read child's stdout
// these read/write ops should go through epoll?!
std::string Cgi::exec_parent_(int pid)
{
  if (this->body_.length() > 0) {
    close(this->pipe_in_[0]);
    if (write(this->pipe_in_[1], this->body_.c_str(), this->body_.length()) == -1) { // this won't work for cte
      throw (ErrorResponse(500));
    }
    close(this->pipe_in_[1]);
  }
  close(this->pipe_out_[1]);
  waitpid(pid, nullptr, 0);

  std::stringstream body;
  const ssize_t buf_size = 4096; // what would be optimal here?
  char buf[buf_size];
  ssize_t read_bytes;
  do {
    read_bytes = read(this->pipe_out_[0], buf, buf_size);
    if (read_bytes < 0)
      throw (ErrorResponse(500));
    body.write(buf, read_bytes);
  } while (read_bytes == buf_size);

  close(this->pipe_out_[0]);
  return (body.str());
}

std::string Cgi::execute_()
{
  // open pipes, input pipe is only necessary if there is a body to write
  // both stdin are redirected the the other process stdout
  if (this->body_.length() > 0) {
    if (pipe(this->pipe_in_) == -1) {
      throw (ErrorResponse(500));
    }
  }
  if (pipe(this->pipe_out_) == -1) {
    throw (ErrorResponse(500));
  }

  // fork and run the parent and child process in separate functions
  int pid = fork();
  if (pid < 0) {
    throw (ErrorResponse(500));
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
    throw (ErrorResponse(500));
  }
  body_begin += 2;
  char* dup;
  try {
    dup = new char[raw.length() - body_begin];
  }
  catch (std::exception&) {
    throw (ErrorResponse(500));
  }
  raw.copy(dup, raw.length(), body_begin);
  res.setBody(dup, raw.length() - body_begin);
  size_t substr_start = raw.find("Content-Type: ") + std::string("Content-Type: ").length();
  size_t substr_len = raw.find(';', substr_start) - substr_start; // spec does not specify ";" as delimiter?
  std::string content_type = raw.substr(substr_start, substr_len);
  res.addHeader("Server", "SuperWebserv10K/0.9.1 (Unix)");
  res.addHeader("Content-Length", std::to_string(raw.length() - body_begin));
  res.addHeader("Content-Type", content_type);
  res.setMessage(200);
}

// function to process raw cgi local redirect response into http response
// this function has not been tested at all!!!
void Cgi::makeLocalRedirResponse_(const std::string& raw, Response& res, Request& req)
{
  req.setUri(st_find_header_value(raw, "Location: "));
  (void) res;
  // TODO: FIX PLS
  // RequestHandler rh(req);
  // rh.execRequest();
  // res = rh.getResponse();
}

// function to process raw cgi client redirect response into http response
void Cgi::makeClientRedirResponse_(const std::string& raw, Response& res)
{
  // This is done to add the 302 body!
  res = ErrorResponse(302);
  res.addHeader("Location", st_find_header_value(raw, "Location: "));
  res.addHeader("Server", "SuperWebserv10K/0.9.1 (Unix)");
  // does not contain body
  if (raw.find("Content-Type") == std::string::npos) {
    return;
  }
  // contains body
  res.addHeader("Content-Type", st_find_header_value(raw, "Content-Type: "));
  const std::string body = raw.substr(util::find_header_end(raw) + 2);
  res.addHeader("Content-Length", std::to_string(body.length()));
  char* dup;
  try {
    dup = new char[body.length() + 1];
  }
  catch (std::exception&) {
    throw (ErrorResponse(500));
  }
  body.copy(dup, body.length());
  dup[body.length()] = '\0';
  res.setBody(dup, body.length());
}
