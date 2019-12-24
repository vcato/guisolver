#include "taggedvalueio.hpp"

#include <sstream>


int main()
{
  std::istringstream stream("x: -1.5");
  ScanTaggedValueResult result = scanTaggedValueFrom(stream);
  assert(result.isValue());
  const TaggedValue &value = result.asValue();
  assert(value.value.isNumeric());
}
