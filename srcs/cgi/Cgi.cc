#include <array>
#include <unistd.h>
#include <iostream>
#include "Cgi.h"
#include "http/ErrorResponse.h"
#include "http/RequestHandler.h"

static void st_del_arr(char** arr)
{
  size_t i = 0;
  while (arr[i] != NULL) {
    delete[] arr[i];
    i++;
  }
  delete[] arr;
}

// make environment variables as specified in CGI RFC
// some of them are missing due to not being required by subject
static char** st_make_env(Request& req)
{
  char** env;
  std::array<std::string, 16> arr = {
    std::string("AUTH_TYPE=\"\""),
    std::string("CONTENT_LENGTH=") + 
      std::string(req.getHeader("Content-Length") ? req.getHeader("Content-Length") : "0"),
    std::string("CONTENT_TYPE=") + 
      std::string(req.getHeader("Content-Type") ? req.getHeader("Content-Type") : ""),
    std::string("GATEWAY_INTERFACE=CGI/1.1"),
    std::string("PATH_INFO=") + std::string(req.getPath()),
    std::string("PATH_TRANSLATED="), // root path_info based on confi
    std::string("QUERY_STRING=") + ((req.getUri().find('?') == std::string::npos) ? 
      std::string("0") : std::string(req.getUri().substr(req.getUri().find('?')))),
    std::string("REMOTE_ADDR=127.0.0.1"), // for now just hardcode localhost, ask lucas to pass the real thing
    std::string("REMOTE_HOST=") + 
      std::string(req.getHeader("Host") ? req.getHeader("Host") : ""),
    std::string("REMOTE_USER="), // not sure that we need this as we're not doing authentication?
    std::string("REQUEST_METHOD=") + 
      std::string(req.getMethod() == Request::GET ? "GET" : "POST"),
    std::string("SCRIPT_NAME=") + std::string(req.getPath()),
    std::string("SERVER_NAME=SuperWebserv10K/0.9.1 (Unix)"),
    std::string("SERVER_PORT=6969"),
    std::string("SERVER_PROTOCOL=HTTP/1.1"),
    std::string("SERVER_SOFTWARE=SuperWebserv10K/0.9.1 (Unix)")
  };
  try {
    env = new char*[arr.size() + 1];
    for (size_t i = 0; i <= arr.size(); i++) {
      env[i] = NULL;
    }
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

Cgi::Cgi(Request& req) :
    body_(req.getBody()),
    path_("." + req.getPath()), // fix getPath() !!! (or confirm that it's working)
    envp_(st_make_env(req))
{}

Cgi::~Cgi()
{
  st_del_arr(this->envp_);
}

// read from parent process on stdin
// execute script
// write output of script back to parent
// make sure to test for leaking fd's!!!
void Cgi::exec_child()
{
  if (this->body_.length() > 0) {
    dup2(this->pipe_in_[0], STDIN_FILENO);
    close(this->pipe_in_[0]);
    close(this->pipe_in_[1]);
  }
  dup2(this->pipe_out_[1], STDOUT_FILENO);
  close(this->pipe_out_[0]);
  close(this->pipe_out_[1]);
  execve(this->path_.c_str(), NULL, this->envp_);
  // if we get here execve failed
  throw (ErrorResponse(500)); // this should be propagated to parent
}

// write request body to child's stdin
// wait for child then read child's stdout
// these read/write ops should go through epoll?!
std::string Cgi::exec_parent(int pid)
{
  if (this->body_.length() > 0) {
    close(this->pipe_in_[0]);
    if (write(this->pipe_in_[1], this->body_.c_str(), this->body_.length()) == -1) { // this won't work for cte
      throw (ErrorResponse(500));
    }
    close(this->pipe_in_[1]);
  }
  close(this->pipe_out_[1]);
  waitpid(pid, NULL, 0);
  const size_t buf_size = 64; // what would be optimal here?
  char buf[buf_size];
  for (size_t i = 0; i < buf_size; i++) { buf[i] = '\0'; }
  std::string result;
  int bytes;
  do {
    bytes = read(this->pipe_out_[0], buf, buf_size - 1);
    if (bytes < 0) {
      throw (ErrorResponse(500));
    }
    buf[bytes] = '\0';
    result.append(buf);
  } while (bytes > 0);
  close(this->pipe_out_[0]);
  return (result);
}

std::string Cgi::execute()
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
    exec_child();
  }
  std::string result = exec_parent(pid);
  return (result);
}

// function to process raw cgi document response into http response
void Cgi::makeDocumentResponse(const std::string& raw, Response& res)
{
  size_t body_begin = raw.find("\n\n");
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
  size_t substr_len = raw.find(";", substr_start) - substr_start; // spec does not specify ";" as delimiter?
  std::string content_type = raw.substr(substr_start, substr_len);
  res.addHeader("Server", "SuperWebserv10K/0.9.1 (Unix)");
  res.addHeader("Content-Length", std::to_string(raw.length() - body_begin));
  res.addHeader("Content-Type", content_type);
  res.setMessage(200);
}

// function to process raw cgi local redirect response into http response
// this function has not been tested at all!!!
void Cgi::makeLocalRedirResponse(const std::string& raw, Response& res, Request& req)
{
  req.setUri(raw);
  RequestHandler rh(req);
  rh.execRequest();
  res = rh.getResponse();
}

static const std::string st_find_header_value(const std::string& msg, const std::string& key)
{
  size_t start = msg.find(key) + key.length();
  size_t i = 0;
  while (!std::isspace(msg[start + i])) {
    i++;
  }
  return (msg.substr(start, i));
}

// function to process raw cgi client redirect response into http response
void Cgi::makeClientRedirResponse(const std::string& raw, Response& res)
{
  res.setMessage(302);
  res.addHeader("Location", st_find_header_value(raw, "Location: "));
  res.addHeader("Server", "SuperWebserv10K/0.9.1 (Unix)");
  // does not contain body
  if (raw.find("Content-Type") == std::string::npos) {
    return;
  }
  // contains body
  res.addHeader("Content-Type", st_find_header_value(raw, "Content-Type: "));
  const std::string body = raw.substr(raw.find("\n\n") + 2);
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
