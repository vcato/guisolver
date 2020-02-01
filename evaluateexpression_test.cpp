#include "evaluateexpression.hpp"

#include <sstream>

using std::ostringstream;
using std::string;


namespace {
struct Tester {
  ostringstream error_stream;
  EvaluationEnvironment environment;

  Optional<float> evaluate(const string &expression)
  {
    return evaluateExpression(expression, error_stream, environment);
  }
};
}


static void testValid(const string &expression, float expected_value)
{
  Tester tester;
  Optional<float> arg = tester.evaluate(expression);
  assert(arg == expected_value);
}


static void testInvalid(const string &expression)
{
  Tester tester;
  Optional<float> arg = tester.evaluate(expression);
  assert(!arg);
}


static void testEmptyString()
{
  testInvalid("");
}


static void testVariable()
{
  Tester tester;
  EvaluationEnvironment &environment = tester.environment;
  environment["x"] = 1.5;
  Optional<float> arg = tester.evaluate("x");
  assert(arg == 1.5);
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
  testVariable();
}
