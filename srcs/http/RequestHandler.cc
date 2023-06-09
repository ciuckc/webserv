#include <exception>
#include <sstream>
#include <string>
#include <unistd.h>
#include "RequestHandler.h"
#include "CasesGET.h"

RequestHandler::RequestHandler() {}

RequestHandler::RequestHandler(const RequestHandler&) {}

RequestHandler::~RequestHandler() {}

RequestHandler& RequestHandler::operator=(const RequestHandler&)
{
  return (*this);
}

// read fd contents to a string
static std::string fil2str(int fd, size_t buffer_size)
{
  std::string result;
  ssize_t     bytes = 1;
  char        buf[buffer_size];

  while (bytes)
  {
    bytes = read(fd, buf, buffer_size);
    if (bytes == -1) {
      throw (std::runtime_error("500 internal error"));
    }
    result.append(buf, buffer_size);
  }
  return (result);
}

void  RequestHandler::ReadRequest(int fd)
{
  std::istringstream raw(fil2str(fd, buffer_size_));
  this->request_.parse(raw); 
}

void  RequestHandler::ExecRequest()
{
  switch (this->request_.GetMethod())
  {
    case (Request::GET):
      this->DoGET_();
      break;
    case (Request::POST):
      this->DoPOST_();
      break;
    default:
      std::runtime_error("400 invalid request");
      break;
  }
}

void  RequestHandler::DoGET_()
{
  // call config method to 
  // mod path if necessary
  // check if upload allowed if applicable
  // accepted method for route

  CasesGET cases;
  for (int i = 0; i < cases.size(); i++)
  {
    if (cases[i].test(this->request_))
    {
      cases[i].act(this->request_);
      break;
    }
  }
}

void  RequestHandler::DoPOST_()
{
  // cases:
  // redirect
  // no file
  // file
  // dir
  // fail
}
