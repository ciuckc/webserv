#ifndef CASES_H
#define CASES_H
#include <vector>
#include "ACase.h"
#include "Request.h"

namespace get {
  class CaseRedirect : public ACase {
     bool test(Request& req) const;
     void act(Request& req) const;
  };
  class CaseNoFile : public ACase {
     bool test(Request& req) const;
     void act(Request& req) const;
  };
  class CaseCGI : public ACase {
     bool test(Request& req) const;
     void act(Request& req) const;
  };
  class CaseFile : public ACase {
     bool test(Request& req) const;
     void act(Request& req) const;
  };
  class CaseDir : public ACase {
     bool test(Request& req) const;
     void act(Request& req) const;
  };
  class CaseFail : public ACase {
     bool test(Request& req) const;
     void act(Request& req) const;
  };
};

class Cases {
  public:
    Cases();
    virtual ~Cases();

    typedef std::vector<ACase*>::iterator iterator;
    typedef std::vector<ACase*>::const_iterator citerator;
    iterator  begin();
    iterator  end();
    citerator cbegin() const;
    citerator cend() const;
    size_t    size() const;
    ACase*    operator[](size_t n);

  protected:
    std::vector<ACase*> cases_;

  private:
    Cases(const Cases&); // = delete
    Cases& operator=(const Cases&); // delete
};

class CasesGET : public Cases {
  public:
    CasesGET();
    ~CasesGET();

  private:
    CasesGET(const CasesGET&); // = delete
    CasesGET& operator=(const CasesGET&); // delete
};

class CasesPOST : public Cases {
  public:
    CasesPOST();
    ~CasesPOST();

  private:
    CasesPOST(const CasesPOST&); // = delete
    CasesPOST& operator=(const CasesPOST&); // delete
};

#endif
