#include "evaluateexpression.hpp"

#include "expressionparser.hpp"
#include "vector.hpp"
#include "rangetext.hpp"
#include "parsedouble.hpp"

using std::ostream;
using std::string;
using std::cerr;


template <typename Key, typename Value>
static const Value* getPtrFrom(const std::map<Key, Value> &map, const Key &key)
{
  auto iter = map.find(key);

  if (iter == map.end()) {
    return nullptr;
  }

  return &iter->second;
}


namespace {
struct Evaluator : EvaluatorInterface {
  using Float = double;

  const std::string &expression;
  EvaluationEnvironment &environment;
  ostream &error_stream;
  vector<Float> stack;

  Evaluator(
    const std::string &expression,
    EvaluationEnvironment &environment,
    ostream &error_stream
  )
  : expression(expression),
    environment(environment),
    error_stream(error_stream)
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

  virtual bool evaluateVariable(const StringRange &identifier_range)
  {
    VariableName variable_name = rangeText(identifier_range);
    const NumericValue *variable_ptr = getPtrFrom(environment, variable_name);

    if (variable_ptr) {
      push(*variable_ptr);
      return true;
    }

    error_stream << "Unknown variable: " << variable_name << "\n";
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

  virtual bool evaluateNegation()
  {
    push(-pop());
    return true;
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


Optional<NumericValue>
evaluateExpression(
  const std::string &expression,
  ostream &error_stream,
  EvaluationEnvironment &environment
)
{
  StringIndex index = 0;
  StringParser string_parser(expression, index);
  Evaluator evaluator(expression, environment, error_stream);

  ExpressionParser parser(string_parser, evaluator, error_stream);

  if (!parser.parseExpression()) {
    return {};
  }

  return evaluator.stack.back();
}


Optional<NumericValue>
evaluateExpression(const std::string &expression, ostream &error_stream)
{
  EvaluationEnvironment environment;
  return evaluateExpression(expression, error_stream, environment);
}
