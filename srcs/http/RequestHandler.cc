#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include "RequestHandler.h"
#include "ErrorResponse.h"
#include "Cases.h"

RequestHandler::RequestHandler() {}

RequestHandler::RequestHandler(const Request& req) : request_(req) {}

RequestHandler::RequestHandler(const RequestHandler&) {}

RequestHandler::~RequestHandler() {}

RequestHandler& RequestHandler::operator=(const RequestHandler&)
{
  return (*this);
}

void  RequestHandler::execRequest()
{
  switch (this->request_.getMethod())
  {
    case (Request::GET):
      this->doGET_();
      break;
    case (Request::POST):
      this->doPOST_();
      break;
    default:
      throw (ErrorResponse(400));
      break;
  }
}

Response&& RequestHandler::getResponse()
{
  return std::move(this->response_);
}

void  RequestHandler::doGET_()
{
  // call config method to 
  // mod path if necessary
  // check if upload allowed if applicable
  // accepted method for route

  CasesGET cases; // it's bad to construct this for each request
  for (size_t i = 0; i < cases.size(); i++)
  {
    if (cases[i]->test(this->request_))
    {
      this->response_ = cases[i]->act(this->request_);
      break;
    }
  }
}

void  RequestHandler::doPOST_()
{
  // cases:
  // redirect
  // no file
  // file
  // dir
  // fail
}
