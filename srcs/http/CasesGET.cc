#include <exception>
#include "CasesGET.h"

using namespace get;

CasesGET::CasesGET()
{
  try
  {
    this->cases_.push_back(new CaseRedirect());
    this->cases_.push_back(new CaseNoFile());
    this->cases_.push_back(new CaseCGI());
    this->cases_.push_back(new CaseFile());
    this->cases_.push_back(new CaseDir());
    this->cases_.push_back(new CaseFail());
  }
  catch (std::exception&)
  {
    std::runtime_error("500 internal error");
  }
}

CasesGET::~CasesGET()
{
  while (!this->cases_.empty())
  {
    delete (this->cases_.back());
    this->cases_.pop_back();
  }
}

iterator  CasesGET::begin()
{
  return (this->cases_.begin());
}

iterator  CasesGET::end()
{
  return (this->cases_.end());
}

citerator  CasesGET::cbegin()
{
  return (this->cases_.cbegin());
}

citerator  CasesGET::cend()
{
  return (this->cases_.cend());
}
