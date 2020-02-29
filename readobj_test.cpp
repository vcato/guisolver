#include "objdata.hpp"
#include "readobj.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <cassert>
#include "vector.hpp"

using std::istream;
using std::string;
using std::cerr;
using std::istringstream;


static const char *obj_text =
  "#Name:Cube\n"
  "#Type:Face-specified\n"
  "#Direction:Clockwise\n"
  "\n"
  "v   -0.5    -0.5    -0.5\n"
  "v   -0.5    -0.5    0.5\n"
  "v   -0.5    0.5    -0.5\n"
  "v   -0.5    0.5    0.5\n"
  "v   0.5    -0.5    -0.5\n"
  "v   0.5    -0.5    0.5\n"
  "v   0.5    0.5    -0.5\n"
  "v   0.5    0.5    0.5\n"
  "\n"
  "f   8    4    2    6\n"
  "f   8    6    5    7\n"
  "f   8    7    3    4\n"
  "f   4    3    1    2\n"
  "f   1    3    7    5\n"
  "f   2    1    5    6\n";


int main()
{
  istringstream stream(obj_text);
  ObjData obj_data = readObj(stream);
  assert(obj_data.vertices.size() == 8);
  assert(obj_data.faces.size() == 6);
}
