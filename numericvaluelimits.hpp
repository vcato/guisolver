#ifndef NUMERICVALUELIMITS_HPP_
#define NUMERICVALUELIMITS_HPP_

#include <limits>
#include "numericvalue.hpp"


inline NumericValue noMinimumNumericValue()
{
  return -std::numeric_limits<float>::max();
}


inline NumericValue noMaximumNumericValue()
{
  return std::numeric_limits<float>::max();
}


#endif /* NUMERICVALUELIMITS_HPP_ */
