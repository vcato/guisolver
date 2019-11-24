#include "printindent.hpp"


void printIndent(std::ostream &stream,int indent_level)
{
  for (int i=0; i!=indent_level; ++i) {
    stream << "  ";
  }
}
