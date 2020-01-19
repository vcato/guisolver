#include "parsedouble.hpp"

#include <sstream>


Optional<double> parseDouble(const std::string &arg)
{
  double value = 0;
  std::istringstream stream(arg);
  stream >> value;

  if (!stream) {
    return {};
  }

  return value;
}
