#include "optional.hpp"


inline Optional<int> maybeInt(const std::string &arg)
{
  try {
    return stoi(arg);
  }
  catch (const std::invalid_argument &) {
    return {};
  }
  catch (const std::out_of_range &) {
    return {};
  }
}
