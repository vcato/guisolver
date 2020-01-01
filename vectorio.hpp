#ifndef VECTORIO_HPP_
#define VECTORIO_HPP_

#include <iostream>
#include "vector.hpp"


template <typename T>
std::ostream& operator<<(std::ostream &stream,const vector<T> &value)
{
  bool first = true;

  stream << "[";

  for (auto &x : value) {
    if (!first) {
      stream << ",";
    }
    else {
      first = false;
    }

    stream << x;
  }

  stream << "]";

  return stream;
}


#endif /* VECTORIO_HPP_ */
