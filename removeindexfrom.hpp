#ifndef REMOVEINDEXFROM_HPP_
#define REMOVEINDEXFROM_HPP_

#include "vector.hpp"


template <typename T>
void removeIndexFrom(vector<T> &v, typename vector<T>::size_type i)
{
  v.erase(v.begin() + i);
}


#endif /* REMOVEINDEXFROM_HPP_ */
