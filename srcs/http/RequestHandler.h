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
    void            ReadRequest(int fd);
    void            ExecRequest();
    const Response& GetResponse() const;

  private:
    static const size_t buffer_size_ = 512;
    Request   request_;
    Response  response_;

    void DoGET_();
    void DoPOST_();
};

#endif
