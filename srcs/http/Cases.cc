#include "Cases.h"

Cases::Cases() = default;

Cases::~Cases() = default;

Cases::citerator  Cases::cbegin() const
{
  return (this->cases_.cbegin());
}

Cases::citerator  Cases::cend() const
{
  return (this->cases_.cend());
}

size_t  Cases::size() const
{
  return (this->cases_.size());
}

ACase& Cases::operator[](size_t n) const
{
  return (*this->cases_[n]);
}
