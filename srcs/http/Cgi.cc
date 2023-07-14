#include <array>
#include <unistd.h>
#include "Cgi.h"
#include "ErrorResponse.h"

static void st_del_arr(char** arr)
{
  size_t i = 0;
  while (arr[i] != NULL) {
    delete[] arr[i];
    i++;
  }
  delete[] arr;
}

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
      env[i] = new char[arr[i].length()];
      arr[i].copy(env[i], arr[i].length());
    }
  }
  catch (std::exception&) {
    st_del_arr(env);
    throw (ErrorResponse(500));
  }
  return (env);
}

Cgi::Cgi(Request& req)
{
  this->envp_ = st_make_env(req);
  this->body_ = req.getBody();
}

Cgi::~Cgi()
{
  st_del_arr(this->envp_);
}

void Cgi::exec_child()
{
  
}

std::string Cgi::exec_parent()
{
  if (this->body_.length() > 0) {
    dup2(this->pipe_in_[1], STDOUT_FILENO);
    close(this->pipe_in_[0]);
    close(this->pipe_in_[1]);
  }
  dup2(this->pipe_out_[0], STDIN_FILENO);
  close(this->pipe_out_[0]);
  close(this->pipe_out_[1]);
  std::cout << this->body_;
  waitpid(pid, NULL, 0);
  std::string result;
  std::cin >> result;
  return (result);
}

std::string Cgi::execute()
{
  // open pipes, input pipe is only necessary if there is a body to write
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
    throw(ErrorResponse(500));
  }
  if (pid == 0) {
    exec_child(this);
    return (NULL); // not sure if this is necessary after execve
  }
  return (exec_parent(this));
}
