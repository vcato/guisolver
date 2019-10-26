#include "sequence.hpp"
#include "vector.hpp"


template <typename T>
auto indicesOf(const vector<T> &v) -> Sequence<typename vector<T>::size_type>
{
 return {0, v.size()};
}
