#ifndef REQUESTHANDLER_H
#define REQUESTHANDLER_H
#include "Request.h"
#include "Response.h"

class RequestHandler {
  public:
    RequestHandler(const Request& req) : request_(req) {}
    RequestHandler() = default;
    ~RequestHandler() = default;

    void       execRequest();
    Response&& getResponse();

  private:
    static const size_t buffer_size_ = 512;
    Request   request_;
    Response  response_;

    // some kind of method that finds all servers listening to used socket 
    // and returns the one with the server_name specified in the HOST header
    bool legalMethod_() const;
    bool isRedirect_() const;
    void handleDir_(std::string& path);
    void handleFile_(std::string& path);
    void autoIndex_(std::string& path);

    RequestHandler(const RequestHandler& that) = delete;
    RequestHandler& operator=(const RequestHandler& that) = delete;
};

#endif
