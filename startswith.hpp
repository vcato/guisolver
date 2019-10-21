#include <algorithm>


template <typename Container>
bool startsWith(const Container &whole,const Container &prefix)
{
  if (prefix.size() > whole.size()) {
    return false;
  }

  return
    std::equal(
      whole.begin(), whole.begin() + prefix.size(),
      prefix.begin(), prefix.end()
    );
}
