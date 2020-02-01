#include <map>
#include "optional.hpp"
#include "variablename.hpp"
#include "numericvalue.hpp"


using EvaluationEnvironment = std::map<VariableName, NumericValue>;


extern Optional<NumericValue>
  evaluateExpression(const std::string &, std::ostream &error_stream);

extern Optional<NumericValue>
  evaluateExpression(
    const std::string &,
    std::ostream &error_stream,
    EvaluationEnvironment &
  );
