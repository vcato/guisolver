#ifndef NUMERICVALUE_HPP_
#define NUMERICVALUE_HPP_


typedef float NumericValue;


inline float noMinimumNumericValue()
{
  return -std::numeric_limits<float>::max();
}


inline float noMaximumNumericValue()
{
  return std::numeric_limits<float>::max();
}


#endif /* NUMERICVALUE_HPP_ */
