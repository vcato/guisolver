#include "nextunusedname.hpp"

#include <sstream>
#include "contains.hpp"

using std::ostringstream;


std::string
nextUnusedName(const vector<std::string> &used_names, const std::string &prefix)
{
  for (int id = 1;; ++id) {
    ostringstream name_stream;
    name_stream << prefix << id;
    std::string next_name_to_try = name_stream.str();

    if (!contains(used_names, next_name_to_try)) {
      return next_name_to_try;
    }
  }
}
