#ifndef REQUESTHANDLER_H
#define REQUESTHANDLER_H
#include "Request.h"
#include "Response.h"

class RequestHandler {
  public:
    RequestHandler();
    RequestHandler(const RequestHandler& that);
    ~RequestHandler();

    RequestHandler& operator=(const RequestHandler& that);
    // maybe some return values or something to know it's ok to proceed?
    void            readRequest(int fd);
    void            execRequest();
    const Response& getResponse() const;

  private:
    static const size_t buffer_size_ = 512;
    Request   request_;
    Response  response_;

    void doGET_();
    void doPOST_();
};

#endif
