#include "vector.hpp"


template <typename T>
size_t findNull(const vector<T*> &v)
{
  return findIndex(v, nullptr);
}


template <typename T>
size_t storeIn(vector<T*> &v, T &x)
{
  assert(!contains(v,&x));
  size_t index = findNull(v);

  if (index == v.size()) {
    v.push_back(&x);
  }
  else {
    v[index] = &x;
  }

  return index;
}
