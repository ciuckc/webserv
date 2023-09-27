#ifndef CGI_H
#define CGI_H
#include <string>
#include "config/ConfigServer.h"
#include "io/Connection.h"
#include "http/Request.h"
#include "http/Response.h"

class Cgi {
  public:
    Cgi(const ConfigServer& config_server, Connection& conn, const std::string& path);
    ~Cgi() = default;
    void act(const std::string& headers);
    static std::string getScriptName(const std::string& path);
    static char** makeEnv(const Request& req, const std::string& path, const ConfigServer& cfg);
    static void delEnv(char** arr);

  private:
    void makeDocumentResponse_(const std::string& headers);
    void makeLocalRedirResponse_(const std::string& headers);
    void makeClientRedirResponse_(const std::string& headers);
    std::string findHeaderValue_(const std::string& headers, const std::string& key);
    void bufferResponse_(const Response& res);

    const ConfigServer& cfg_;
    Connection& conn_;
    const std::string& path_;
};

#endif
