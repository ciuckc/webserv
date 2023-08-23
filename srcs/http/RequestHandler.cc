#include <exception>
#include "RequestHandler.h"
#include "ErrorResponse.h"
#include "Cases.h"

RequestHandler::RequestHandler() = default;

RequestHandler::RequestHandler(const Request& req) : request_(req) {}

RequestHandler::RequestHandler(const RequestHandler&) {}

RequestHandler::~RequestHandler() = default;

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

  const Cases& cases = Case::instance.get_instance;
  for (auto it = cases.cbegin(); it < cases.cend(); it++) {
    const std::unique_ptr<ACase>& ptr = *it;
    if (ptr->test(request_)) {
      response_ = ptr->act(request_);
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
