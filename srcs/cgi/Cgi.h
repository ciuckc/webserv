#ifndef CGI_H
#define CGI_H
#include <string>
#include "http/Request.h"
#include "http/Response.h"

class Cgi {
  public:
    Cgi(Request&);
    ~Cgi();

    std::string execute();
    static void makeDocumentResponse(const std::string&, Response&);
    static void makeLocalRedirResponse(const std::string&, Response&);
    static void makeClientRedirResponse(const std::string&, Response&);

  private:
    const std::string body_;
    const std::string path_;
    char**            envp_;
    int               pipe_in_[2];
    int               pipe_out_[2];

    void exec_child();
    std::string exec_parent(int pid);
};

#endif
