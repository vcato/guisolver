#include "expressionparser.hpp"

using std::string;


static bool skipNumber(const StringParser &string_parser)
{
  if (!string_parser.isDigit(string_parser.peekChar())) return false;

  while (string_parser.isDigit(string_parser.peekChar())) {
    string_parser.skipChar();
  }

  if (string_parser.peekChar() == '.') {
    string_parser.skipChar();

    while (string_parser.isDigit(string_parser.peekChar())) {
      string_parser.skipChar();
    }
  }

  return true;
}


static Optional<StringRange>
maybeNumberRange(const StringParser &string_parser)
{
  string_parser.skipWhitespace();
  StringIndex start = string_parser.index();

  if (!skipNumber(string_parser)) {
    return {};
  }

  StringIndex end = string_parser.index();
  return StringRange{start, end};
}


void ExpressionParser::skipChar() const
{
  string_parser.skipChar();
}


void ExpressionParser::skipWhitespace() const
{
  string_parser.skipWhitespace();
}


char ExpressionParser::peekChar() const
{
  return string_parser.peekChar();
}


bool
  ExpressionParser::parsePrimaryStartingWithIdentifier(
    const StringRange &identifier_range
  ) const
{
  return evaluator.evaluateVariable(identifier_range);
}


bool ExpressionParser::parsePrimary() const
{
  if (peekChar()=='(') {
    skipChar();

    if (!parseExpression()) {
      return {};
    }

    if (peekChar() != ')') {
      error_stream << "Missing ')'\n";
      return {};
    }

    skipChar();
    return true;
  }

  if (Optional<StringRange> maybe_range = maybeNumberRange(string_parser)) {
    return evaluator.evaluateNumber(*maybe_range);
  }

  if (peekChar()=='$') {
    skipChar();
    return evaluator.evaluateDollar();
  }

  if (peekChar()=='[') {
    skipChar();

    int n_elements = 0;

    if (peekChar() != ']') {
      for (;;) {
        if (!parseExpression()) {
          return false;
        }

        ++n_elements;

        if (peekChar()==']') {
          break;
        }

        if (peekChar()==',') {
          skipChar();
        }
        else {
          //assert(false); // not tested
        }
      }
    }

    assert(peekChar() == ']');
    skipChar();

    if (!evaluator.evaluateVector(n_elements)) {
      return false;
    }

    return true;
  }

  Optional<StringRange> maybe_identifier_range =
    string_parser.maybeIdentifierRange();

  if (maybe_identifier_range) {
    return parsePrimaryStartingWithIdentifier(*maybe_identifier_range);
  }

  if (string_parser.atEnd()) {
    error_stream << "Unexpected end of expression\n";
    return false;
  }

  error_stream << "Unexpected '" << peekChar() << "'\n";

  return false;
}


bool
  ExpressionParser::parseStartingWithIdentifier(
    const StringRange &identifier_range
  ) const
{
  return
    parsePrimaryStartingWithIdentifier(identifier_range) &&
    extendPostfix() &&
    extendFactor() &&
    extendTerm();
}


bool ExpressionParser::parseFunctionArgument() const
{
  Optional<StringParser::Range> maybe_identifier_range =
    string_parser.maybeIdentifierRange();

  if (!maybe_identifier_range) {
    evaluator.evaluateNoName();

    if (!parseExpression()) {
      return false;
    }
  }
  else {
    if (peekChar() == '=') {
      skipChar();
      evaluator.evaluateName(*maybe_identifier_range);

      if (!parseExpression()) {
        return false;
      }
    }
    else {
      evaluator.evaluateNoName();

      if (!parseStartingWithIdentifier(*maybe_identifier_range)) {
        return false;
      }
    }
  }

  return true;
}


bool ExpressionParser::parseFunctionArguments(int &n_arguments) const
{
  skipWhitespace();

  if (peekChar()==')') {
    return true;
  }

  for (;;) {
    if (!parseFunctionArgument()) {
      return false;
    }

    ++n_arguments;

    if (peekChar() == ',') {
      skipChar();
    }
    else if (peekChar() == ')') {
      break;
    }
    else {
      error_stream << "Missing ','\n";
      return false;
    }
  }

  return true;
}


bool ExpressionParser::extendPostfix() const
{
  for (;;) {
    skipWhitespace();

    if (peekChar()=='.') {
      skipChar();

      Optional<StringParser::Range> maybe_identifier_range =
        string_parser.maybeIdentifierRange();

      if (!maybe_identifier_range) {
        return false;
      }

      if (!evaluator.evaluateMember(*maybe_identifier_range)) {
        return false;
      }
    }
    else if (peekChar()=='(') {
      skipChar();

      int n_arguments = 0;

      if (!parseFunctionArguments(n_arguments)) {
        return false;
      }

      assert(peekChar() == ')');
      skipChar();

      if (!evaluator.evaluateCall(n_arguments)) {
        return false;
      }
    }
    else {
      break;
    }
  }

  return true;
}


bool ExpressionParser::extendTerm() const
{
  for (;;) {
    skipWhitespace();

    if (peekChar()=='+') {
      skipChar();

      if (!parseFactor()) {
        return false;
      }

      if (!evaluator.evaluateAddition()) {
        return false;
      }
    }
    else if (peekChar()=='-') {
      skipChar();

      if (!parseFactor()) {
        return false;
      }

      if (!evaluator.evaluateSubtraction()) {
        return false;
      }
    }
    else {
      break;
    }
  }

  return true;
}


bool ExpressionParser::extendFactor() const
{
  for (;;) {
    if (peekChar()=='*') {
      skipChar();

      if (!parsePostfix()) {
        return false;
      }

      if (!evaluator.evaluateMultiplication()) {
        return false;
      }
    }
    else if (peekChar()=='/') {
      skipChar();

      if (!parsePostfix()) {
        return false;
      }

      if (!evaluator.evaluateDivision()) {
        return false;
      }
    }
    else {
      break;
    }
  }

  return true;
}


bool ExpressionParser::parsePostfix() const
{
  return parsePrimary() && extendPostfix();
}


bool ExpressionParser::parseFactor() const
{
  return parsePostfix() && extendFactor();
}


bool ExpressionParser::parseTerm() const
{
  return parseFactor() && extendTerm();
}


bool ExpressionParser::parseExpression() const
{
  return parseTerm();
}
