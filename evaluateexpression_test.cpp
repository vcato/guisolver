#include "evaluateexpression.hpp"

#include <sstream>

using std::ostringstream;
using std::string;


static void testValid(const string &expression, float expected_value)
{
  ostringstream error_stream;
  Optional<float> arg = evaluateExpression(expression, error_stream);
  assert(arg == expected_value);
}


static void testInvalid(const string &expression)
{
  ostringstream error_stream;
  Optional<float> arg = evaluateExpression(expression, error_stream);
  assert(!arg);
}


static void testEmptyString()
{
  testInvalid("");
}


int main()
{
  testValid("5.25", 5.25);
  testValid("2.5 + 3.25", 2.5 + 3.25);
  testValid("1/2", 1/2.0);
  testValid("1.5-2", -0.5);
  testValid("1.5*2", 3.0);
  testInvalid("blah");
  testInvalid("1..2");
  testInvalid("$");
  testInvalid("[2,3]");
  testInvalid("(2).x");
  testInvalid("(2)(2)");
  testInvalid("(2)(x=2)");
  testEmptyString();
}
