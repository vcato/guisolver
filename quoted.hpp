#ifndef QUOTED_HPP_
#define QUOTED_HPP_

#include <string>


inline std::string quoted(const std::string &s)
{
  return '"' + s + '"';
}


#endif /* QUOTED_HPP_ */
