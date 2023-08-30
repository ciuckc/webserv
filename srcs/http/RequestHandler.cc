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
  switch (request_.getMethod())
  {
    case (HTTP::GET):
      doGET_();
      break;
    case (HTTP::POST):
      doPOST_();
      break;
    default:
      response_ = ErrorResponse(400);
      return;
  }
}

Response&& RequestHandler::getResponse()
{
  return std::move(response_);
}

void  RequestHandler::doMethod_(const Cases& cases)
{
  for (auto it = cases.cbegin(); it < cases.cend(); it++) {
    const std::unique_ptr<ACase>& ptr = *it;
    if (ptr->test(request_)) {
      response_ = ptr->act(request_);
      break;
    }
  }
}

void  RequestHandler::doGET_()
{
  // filter all location-based requirements here
  const Cases& cases = Case::instance.get_instance;
  doMethod_(cases);
}

void  RequestHandler::doPOST_()
{
  // filter all location-based requirements here
  const Cases& cases = Case::instance.post_instance;
  doMethod_(cases);
}
