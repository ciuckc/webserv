#ifndef REQUESTHANDLER_H
#define REQUESTHANDLER_H
#include "Request.h"
#include "Response.h"
#include "Cases.h"

class RequestHandler {
  public:
    RequestHandler();
    RequestHandler(const Request&);
    RequestHandler(const RequestHandler& that);
    ~RequestHandler();

    RequestHandler& operator=(const RequestHandler& that);
    // maybe some return values or something to know it's ok to proceed?
    // void            readRequest(int fd);
    void            execRequest();
    Response&& getResponse();

  private:
    static const size_t buffer_size_ = 512;
    Request   request_;
    Response  response_;

    void doMethod_(const Cases& cases);
    void doGET_();
    void doPOST_();
};

#endif
