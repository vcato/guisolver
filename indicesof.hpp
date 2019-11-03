#ifndef INDICESOF_HPP_
#define INDICESOF_HPP_

#include "sequence.hpp"
#include "vector.hpp"


template <typename T>
auto indicesOf(const vector<T> &v) -> Sequence<typename vector<T>::size_type>
{
 return {0, v.size()};
}


#endif /* INDICESOF_HPP_ */
