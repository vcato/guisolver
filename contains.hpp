#ifndef CONTAINS_HPP_
#define CONTAINS_HPP_

#include <algorithm>
#include "vector.hpp"


template <typename A,typename B>
static size_t findIndex(const vector<A> &v,B p)
{
  auto match_function = [&](const A &a){ return a==p; };
  return std::find_if(v.begin(), v.end(), match_function) - v.begin();
}


template <typename T>
static bool contains(const vector<T> &v,T p)
{
  return findIndex(v,p) != v.size();
}


#endif /* CONTAINS_HPP_ */
