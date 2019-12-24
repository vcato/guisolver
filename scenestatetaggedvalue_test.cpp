#include "scenestatetaggedvalue.hpp"

#include <sstream>
#include <iostream>
#include "taggedvalueio.hpp"

using std::istringstream;
using std::cerr;


int main()
{
  const char *text =
    "Transform {\n"
    "  translation {\n"
    "    x: 0\n"
    "    y: 0\n"
    "    z: 0\n"
    "  }\n"
    "  rotation {\n"
    "    x: 137.922\n"
    "    y: -58.6023\n"
    "    z: 2.99252e-05\n"
    "  }\n"
    "  Box {\n"
    "    scale_x: 1\n"
    "    scale_y: 2.5\n"
    "    scale_z: 3.5\n"
    "  }\n"
    "}\n";

  istringstream stream(text);
  ScanTaggedValueResult result = scanTaggedValueFrom(stream);
  assert(result.isValue());
  SceneState state;
  const TaggedValue &tagged_value = result.asValue();

  createBodyFromTaggedValue(
    state,
    tagged_value,
    /*maybe_parent_index*/{}
  );

  assert(state.bodies().size() == 1);
  assert(state.body(0).scale.x == 1);
  assert(state.body(0).scale.y == 2.5);
  assert(state.body(0).scale.z == 3.5);
}
