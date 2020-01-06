#include <string>
#include "vector.hpp"


extern std::string
  nextUnusedName(
    const vector<std::string> &used_names, const std::string &prefix
  );
