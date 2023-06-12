#include <exception>
#include "Cases.h"

Cases::Cases()
{
}

Cases::~Cases()
{
}

Cases::iterator  Cases::begin()
{
  return (this->cases_.begin());
}

Cases::iterator  Cases::end()
{
  return (this->cases_.end());
}

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

ACase*  Cases::operator[](size_t n)
{
  return (this->cases_[n]);
}
