#include <algorithm>
#include "vector.hpp"


template <typename A,typename B>
static size_t findIndex(const vector<A> &v,B p)
{
  return std::find(v.begin(),v.end(),p) - v.begin();
}


template <typename T>
static bool contains(const vector<T> &v,T p)
{
  return findIndex(v,p) != v.size();
}
