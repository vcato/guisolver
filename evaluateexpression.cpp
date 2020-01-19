#include "evaluateexpression.hpp"

#include "expressionparser.hpp"
#include "vector.hpp"
#include "rangetext.hpp"
#include "parsedouble.hpp"

using std::ostream;
using std::string;


namespace {
struct Evaluator : EvaluatorInterface {
  const std::string &expression;
  using Float = double;
  vector<Float> stack;

  Evaluator(const std::string &expression)
  : expression(expression)
  {
  }

  Float pop()
  {
    Float result = stack.back();
    stack.pop_back();
    return result;
  }

  void push(Float arg)
  {
    stack.push_back(arg);
  }

  string rangeText(const StringRange &number_range)
  {
    return ::rangeText(number_range, expression);
  }

  virtual bool evaluateVariable(const StringRange &/*identifier_range*/)
  {
    // No scope
    return false;
  }

  virtual bool evaluateNumber(const StringRange &number_range)
  {
    Optional<Float> maybe_value = parseDouble(rangeText(number_range));

    if (!maybe_value) {
      // It's very difficult to have a number which looks valid, but isn't
      // actually valid.  It would have to be something so large that it
      // wouldn't fit in a double, but we don't support exponential notation.
      assert(false); // not implemented
    }

    push(*maybe_value);
    return true;
  }

  virtual bool evaluateDollar()
  {
    return false;
  }

  virtual bool evaluateVector(int /*n_elements*/)
  {
    return false;
  }

  virtual bool evaluateAddition()
  {
    Float b = pop();
    Float a = pop();
    push(a + b);
    return true;
  }

  virtual bool evaluateSubtraction()
  {
    Float b = pop();
    Float a = pop();
    push(a - b);
    return true;
  }

  virtual bool evaluateMultiplication()
  {
    Float b = pop();
    Float a = pop();
    push(a * b);
    return true;
  }

  virtual bool evaluateDivision()
  {
    Float b = pop();
    Float a = pop();
    push(a / b);
    return true;
  }

  virtual bool evaluateMember(const StringRange &/*name_range*/)
  {
    return false; // not supported
  }

  virtual void evaluateNoName()
  {
    // functions are not supported
  }

  virtual void evaluateName(const StringRange &/*range*/)
  {
    // functions are not supported
  }

  virtual bool evaluateCall(const int /*n_arguments*/)
  {
    return false; // functions are not supported
  }
};
}


Optional<float>
evaluateExpression(const std::string &expression, ostream &error_stream)
{
  StringIndex index = 0;
  StringParser string_parser(expression, index);
  Evaluator evaluator(expression);

  ExpressionParser parser(string_parser, evaluator, error_stream);

  if (!parser.parseExpression()) {
    return {};
  }

  return evaluator.stack.back();
}
