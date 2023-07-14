#ifndef CGI_H
#define CGI_H
#include <string>
#include "Request.h"

class Cgi {
  public:
    Cgi(Request&);
    ~Cgi();
    std::string execute();

  private:
    std::string body_;
    char**      envp_;
    int         pipe_in_[2];
    int         pipe_out_[2];

    void exec_child();
    std::string exec_parent();
};

#endif
