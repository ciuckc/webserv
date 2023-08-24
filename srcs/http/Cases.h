#ifndef CASES_H
#define CASES_H
#include <vector>
#include <memory>
#include "ACase.h"
#include "Request.h"

namespace get {
  class CaseRedirect : public ACase {
    bool test(Request& req) const override;
    Response act(Request& req) const override;
  };
  class CaseNoFile : public ACase {
    bool test(Request& req) const override;
    Response act(Request& req) const override;
  };
  class CaseCGI : public ACase {
    bool test(Request& req) const override;
    Response act(Request& req) const override;
  };
  class CaseFile : public ACase {
    bool test(Request& req) const override;
    Response act(Request& req) const override;
  };
  class CaseDir : public ACase {
    bool test(Request& req) const override;
    Response act(Request& req) const override;
  };
  class CaseFail : public ACase {
    bool test(Request& req) const override;
    Response act(Request& req) const override;
  };
};

namespace post {
  using get::CaseRedirect;
  using get::CaseCGI;
  using get::CaseNoFile;
  using get::CaseFail;
  class CaseStaticContent : public ACase {
    bool test(Request& req) const override;
    Response act(Request& req) const override;
  };
}

class Cases {
 public:
  Cases();
  virtual ~Cases();

  typedef std::vector<std::unique_ptr<ACase> >::const_iterator citerator;
  citerator cbegin() const;
  citerator cend() const;
  size_t    size() const;
  ACase&    operator[](size_t n) const;

 protected:
  std::vector<std::unique_ptr<ACase> > cases_;

 private:
  Cases(const Cases&); // = delete
  Cases& operator=(const Cases&); // delete


};

class CasesGET : public Cases {
 public:
  CasesGET();
  ~CasesGET() override = default;
  CasesGET(const CasesGET&) = delete;
  CasesGET& operator=(const CasesGET&) = delete;
};

class CasesPOST : public Cases {
 public:
  CasesPOST();
  ~CasesPOST() override = default;
  CasesPOST(const CasesPOST&) = delete;
  CasesPOST& operator=(const CasesPOST&) = delete;
};

namespace Case {
static struct {
  const CasesGET get_instance;
  const CasesPOST post_instance;
} instance{};
}

#endif
