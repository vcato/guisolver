#include "vector.hpp"


template <typename T, typename... Args>
void
emplaceInto(vector<T> &v, typename vector<T>::size_type index, Args&&... args)
{
  assert(index == v.size());
  v.emplace_back(std::forward<Args>(args)...);
}
