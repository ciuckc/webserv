#ifndef CGI_H
#define CGI_H
#include <string>
#include "http/Request.h"
#include "http/Response.h"

class Cgi {
  public:
    Cgi(const Request&);
    ~Cgi();
    Response act();

  private:
    std::string execute_();
    std::string exec_parent_(int pid);
    void exec_child_();
    static void makeDocumentResponse_(const std::string&, Response&);
    static void makeLocalRedirResponse_(const std::string&, Response&, Request&);
    static void makeClientRedirResponse_(const std::string&, Response&);

    const std::string body_;
    const std::string path_;
    char**            envp_;
    int               pipe_in_[2];
    int               pipe_out_[2];
    Request           req_; // feels a bit wasteful to have this just for local redir
};

#endif
