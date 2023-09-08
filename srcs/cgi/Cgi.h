#ifndef CGI_H
#define CGI_H
#include <string>
#include "http/RequestHandler.h"
#include "http/Request.h"
#include "http/Response.h"

class Cgi {
  public:
    Cgi(RequestHandler&, const std::string&);
    ~Cgi();
    void act();

  private:
    std::string execute_();
    std::string exec_parent_(int pid);
    void exec_child_();
    void makeDocumentResponse_(const std::string&, Response&);
    void makeLocalRedirResponse_(const std::string&, Response&, Request&);
    void makeClientRedirResponse_(const std::string&, Response&);

    RequestHandler& rh_;
    const std::string& path_;
    char** envp_;
    int    pipe_in_[2];
    int    pipe_out_[2];
};

#endif
