#include "expressionparser.hpp"

#include <sstream>
#include "rangetext.hpp"


using std::string;
using std::istringstream;
using std::ostringstream;
using std::cerr;


namespace {
struct FakeEvaluator : EvaluatorInterface {
  struct Flags {
    bool evaluate_number_fails = false;
    bool evaluate_dollar_fails = false;
    bool allow_empty_vector = true;
    bool multiply_fails = false;
  };

  StringParser &string_parser;
  ostringstream &command_stream;
  ostringstream &error_stream;
  const Flags &flags;

  FakeEvaluator(
    StringParser &string_parser,
    ostringstream &command_stream,
    ostringstream &error_stream,
    const Flags &flags
  )
  : string_parser(string_parser),
    command_stream(command_stream),
    error_stream(error_stream),
    flags(flags)
  {
  }

  string rangeText(const StringRange &number_range)
  {
    return ::rangeText(number_range, string_parser.text);
  }

  virtual bool evaluateVariable(const StringRange &identifier_range)
  {
    command_stream << "variable(" << rangeText(identifier_range) << ")\n";
    return true;
  }

  virtual bool evaluateNumber(const StringRange &number_range)
  {
    command_stream << "number(" << rangeText(number_range) << ")\n";

    if (flags.evaluate_number_fails) {
      return false;
    }

    return true;
  }

  virtual bool evaluateDollar()
  {
    command_stream << "dollar()\n";

    if (flags.evaluate_dollar_fails) {
      return false;
    }

    assert(false); // not implemented
  }

  virtual bool evaluateVector(int n_elements)
  {
    command_stream << "vector(n_elements=" << n_elements << ")\n";

    if (!flags.allow_empty_vector) {
      if (n_elements == 0) {
        error_stream << "Empty vector not allowed\n";
        return false;
      }
    }

    return true;
  }

  virtual bool evaluateAddition()
  {
    command_stream << "add()\n";
    return true;
  }

  virtual bool evaluateSubtraction()
  {
    assert(false); // not implemented
  }

  virtual bool evaluateMultiplication()
  {
    command_stream << "multiply()\n";

    if (flags.multiply_fails) {
      error_stream << "Multiply failed\n";
      return false;
    }

    return true;
  }

  virtual bool evaluateDivision()
  {
    assert(false); // not implemented
  }

  virtual bool evaluateMember(const StringRange &/*name_range*/)
  {
    return true;
  }

  virtual void evaluateNoName()
  {
    command_stream << "noName()\n";
  }

  virtual void evaluateName(const StringRange &/*range*/)
  {
    assert(false); // not implemented
  }

  virtual bool evaluateCall(const int n_arguments)
  {
    command_stream << "call(n_arguments=" << n_arguments << ")\n";
    return true;
  }
};
}


namespace {
struct Tester {
  ostringstream command_stream;
  ostringstream error_stream;
  FakeEvaluator::Flags evaluator_flags;
  StringIndex index = 0;

  bool parseExpression(const string &expression)
  {
    StringParser string_parser(expression, index);

    FakeEvaluator
      evaluator(string_parser, command_stream, error_stream, evaluator_flags);

    ExpressionParser parser(string_parser, evaluator, error_stream);
    return parser.parseExpression();
  }

  bool hitEndOfExpression(const string &expression)
  {
    return (index == StringIndex(expression.size()));
  }

  void parseValidExpression(const string &expression)
  {
    bool was_valid = parseExpression(expression);
    assert(was_valid);
    assert(errorString() == "");
    assert(hitEndOfExpression(expression));
  }

  void parseInvalidExpression(const string &expression)
  {
    bool was_valid = parseExpression(expression);

    if (!was_valid) {
      return;
    }

    if (!hitEndOfExpression(expression)) {
      return;
    }

    assert(false);
  }

  string commandString()
  {
    return command_stream.str();
  }

  string errorString()
  {
    return error_stream.str();
  }
};
}


static bool
checkEqual(const string &error_string, const string &expected_error_string)
{
  if (error_string != expected_error_string) {
    cerr << "error_string: " << error_string << "\n";
    cerr << "expected:     " << expected_error_string << "\n";
  }

  return (error_string == expected_error_string);
}


// Beginning of tests
/////////////////////


static void testSingleNumber()
{
  const string expression = "1";
  string expected_command_string = "number(1)\n";

  Tester tester;
  tester.parseValidExpression(expression);
  string command_string = tester.commandString();
  assert(command_string == expected_command_string);
}


static void testBadNumber()
{
  const string expression = "1";
  string expected_command_string = "number(1)\n";

  Tester tester;
  tester.evaluator_flags.evaluate_number_fails = true;
  tester.parseInvalidExpression(expression);
  string command_string = tester.commandString();
  assert(command_string == expected_command_string);
}


static void testBadDollar()
{
  const string expression = "$";
  string expected_command_string = "dollar()\n";

  Tester tester;
  tester.evaluator_flags.evaluate_dollar_fails = true;
  tester.parseInvalidExpression(expression);
  string command_string = tester.commandString();
  assert(command_string == expected_command_string);
}


static void testBadMember()
{
  const string expression = "a.5";

  string expected_command_string =
    "variable(a)\n";

  Tester tester;
  tester.parseInvalidExpression(expression);
  string command_string = tester.commandString();
  assert(command_string == expected_command_string);
}


static void testFunctionArgumentWithFactors()
{
  const string expression = "f(a*2)";

  string expected_command_string =
    "variable(f)\n"
    "noName()\n"
    "variable(a)\n"
    "number(2)\n"
    "multiply()\n"
    "call(n_arguments=1)\n";

  Tester tester;
  tester.parseValidExpression(expression);
  string command_string = tester.commandString();
  assert(command_string == expected_command_string);
}


static void testValid(const string &expression)
{
  Tester tester;
  tester.parseValidExpression(expression);
}


static void
testInvalid(const string &expression, const string &expected_error_string)
{
  Tester tester;
  tester.parseInvalidExpression(expression);
  string error_string = tester.errorString();
  assert(checkEqual(error_string, expected_error_string));
}


static void testInvalidWhenEmptyVectorsAreNotAllowed(const string &expression)
{
  Tester tester;

  tester.evaluator_flags.allow_empty_vector = false;
  string expected_error_string = "Empty vector not allowed\n";

  tester.parseInvalidExpression(expression);
  assert(checkEqual(tester.errorString(), expected_error_string));
}


static void testBadVector()
{
  const string expression = "2*[[],2]";
  testInvalidWhenEmptyVectorsAreNotAllowed(expression);
}


static void testBadVectorMultiplication()
{
  Tester tester;

  const string expression = "[]*[]";
  tester.evaluator_flags.multiply_fails = true;
  string expected_error_string = "Multiply failed\n";
  tester.parseInvalidExpression(expression);
  assert(checkEqual(tester.errorString(), expected_error_string));
}


static void testTruncated(const string &expression)
{
  testInvalid(expression, "Unexpected end of expression\n");
}


int main()
{
  testSingleNumber();
  testBadNumber();
  testBadDollar();
  testBadMember();
  testBadVector();
  testBadVectorMultiplication();
  testFunctionArgumentWithFactors();
  testValid("f()+g()");
  testValid("5.2");
  testInvalid("[,2]", "Unexpected ','\n");
  testInvalid("1.x", "");
  testTruncated("[");
  testTruncated("2*");
  testTruncated("[1,2] +");
  testInvalidWhenEmptyVectorsAreNotAllowed("[[],2] + [3,4]");
  testInvalidWhenEmptyVectorsAreNotAllowed("[1,2] + [[],4]");
}
