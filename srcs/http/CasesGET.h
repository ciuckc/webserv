#ifndef CASESGET_H
#define CASESGET_H
#include <vector>
#include "ACase.h"
#include "Request.h"

namespace get {
  class CaseRedirect : public ACase {};
  class CaseNoFile : public ACase {};
  class CaseCGI : public ACase {};
  class CaseFile : public ACase {};
  class CaseDir : public ACase {};
  class CaseFail : public ACase {};
};

class CasesGET {
  public:
    CasesGET();
    ~CasesGET();

    typedef std::vector<ACase*>::iterator itarator;
    typedef std::vector<ACase*>::citerator citarator;
    iterator  begin();
    iterator  end();
    citerator cbegin();
    citerator cend();

  private:
    CasesGET(const CasesGET&); // = delete
    CasesGET& operator=(const CasesGET&); // delete

    std::vector<ACase*> cases_;
};

#endif
