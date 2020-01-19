#include <iostream>
#include "stringrange.hpp"
#include "stringparser.hpp"


struct EvaluatorInterface {
  virtual bool evaluateVariable(const StringRange &identifier_range) = 0;
  virtual bool evaluateNumber(const StringRange &number_range) = 0;
  virtual bool evaluateDollar() = 0;
  virtual bool evaluateVector(int n_elements) = 0;
  virtual bool evaluateAddition() = 0;
  virtual bool evaluateSubtraction() = 0;
  virtual bool evaluateMultiplication() = 0;
  virtual bool evaluateDivision() = 0;
  virtual bool evaluateMember(const StringRange &name_range) = 0;
  virtual void evaluateNoName() = 0;
  virtual void evaluateName(const StringRange &range) = 0;
  virtual bool evaluateCall(const int n_arguments) = 0;
};


class ExpressionParser
{
  public:
    std::ostream &error_stream;
    StringParser &string_parser;
    EvaluatorInterface &evaluator;

    ExpressionParser(
      StringParser &string_parser,
      EvaluatorInterface &evaluator,
      std::ostream &error_stream
    )
    : error_stream(error_stream),
      string_parser(string_parser),
      evaluator(evaluator)
    {
    }

    bool parseExpression() const;
    bool parseStartingWithIdentifier(const StringRange &) const;

  private:
    void skipChar() const;
    char peekChar() const;
    bool parsePrimaryStartingWithIdentifier(const StringRange &) const;
    bool parsePrimary() const;
    bool parsePostfix() const;
    bool parseFactor() const;
    bool parseTerm() const;
    bool extendPostfix() const;
    bool extendFactor() const;
    bool extendTerm() const;
    bool parseFunctionArgument() const;
    bool parseFunctionArguments(int &n_arguments) const;
};
