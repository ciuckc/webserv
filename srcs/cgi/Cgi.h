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
    std::string getScriptName(const std::string&);
    char** makeEnv(const Request&);

  private:
    std::string execute_();
    std::string exec_parent_();
    void exec_child_();
    void makeDocumentResponse_(const std::string&, Response&);
    void makeLocalRedirResponse_(const std::string&, Response&, Request&);
    void makeClientRedirResponse_(const std::string&, Response&);
    // helper functions
    std::string findHeaderValue_(const std::string&, const std::string&);
    void delArr_(char**);

    RequestHandler& rh_;
    const std::string& path_;
    const std::string script_;
    char** envp_;
    int    pipe_in_[2];
    int    pipe_out_[2];
};

#endif
