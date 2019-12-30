#include "scenestatetaggedvalue.hpp"

#include <sstream>
#include <iostream>
#include "taggedvalueio.hpp"

using std::istringstream;
using std::cerr;


static BodyIndex
  createBodyIn(
    SceneState &scene_state,
    Optional<BodyIndex> maybe_parent_index
  )
{
  BodyIndex body_index = scene_state.createBody(maybe_parent_index);
  setAll(scene_state.body(body_index).solve_flags, true);
  return body_index;
}


static BodyIndex createGlobalBodyIn(SceneState &scene_state)
{
  return createBodyIn(scene_state, /*maybe_parent_index*/{});
}


static void testCreatingABodyFromATaggedValue()
{
  const char *text =
    "Transform {\n"
    "  translation {\n"
    "    x: 0 {\n"
    "      solve: false\n"
    "    }\n"
    "    y: 0 {\n"
    "      solve: true\n"
    "    }\n"
    "    y: 0\n"
    "    z: 0\n"
    "  }\n"
    "  rotation {\n"
    "    x: 137.922\n"
    "    y: -58.6023\n"
    "    z: 2.99252e-05\n"
    "  }\n"
    "  Box {\n"
    "    scale {\n"
    "      x: 1\n"
    "      y: 2.5\n"
    "      z: 3.5\n"
    "    }\n"
    "    center {\n"
    "      x: 1.5\n"
    "      y: 2.5\n"
    "      z: 3.5\n"
    "    }\n"
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
  assert(state.body(0).geometry.scale.x == 1);
  assert(state.body(0).geometry.scale.y == 2.5);
  assert(state.body(0).geometry.scale.z == 3.5);
  assert(state.body(0).geometry.center.x == 1.5);
  assert(state.body(0).geometry.center.y == 2.5);
  assert(state.body(0).geometry.center.z == 3.5);
  assert(state.body(0).solve_flags.translation.x == false);
  assert(state.body(0).solve_flags.translation.y == true);
}


static void testBody()
{
  Optional<TaggedValue> maybe_transform_tagged_value;

  {
    SceneState scene_state;

    BodyIndex body_index = createGlobalBodyIn(scene_state);
    scene_state.body(body_index).solve_flags.translation.y = false;
    scene_state.body(body_index).geometry.center.y = 2.5;

    TaggedValue tagged_value("root");
    createBodyTaggedValue(tagged_value, body_index, scene_state);

    const TaggedValue *transform_tagged_value_ptr =
      findChild(tagged_value, "Transform");

    assert(transform_tagged_value_ptr);
    maybe_transform_tagged_value = *transform_tagged_value_ptr;
  }
  {
    SceneState scene_state;

    BodyIndex body_index =
      createBodyFromTaggedValue(
        scene_state,
        *maybe_transform_tagged_value,
        /*maybe_parent_index*/{}
      );

    assert(scene_state.body(body_index).solve_flags.translation.y == false);
    assert(scene_state.body(body_index).geometry.center.y == 2.5);
  }
}


static TaggedValue
createTaggedValueForBody(BodyIndex body_index, const SceneState &scene_state)
{
  TaggedValue root_tag_value("");
  createBodyTaggedValue(root_tag_value, body_index, scene_state);
  return root_tag_value.children[0];
}


static void testCreatingABodyFromATaggedValueWithConflictingMarkerNames()
{
  SceneState scene_state;
  BodyIndex body_index = scene_state.createBody();
  scene_state.createMarker(body_index);
  TaggedValue tagged_value = createTaggedValueForBody(body_index, scene_state);
  createBodyFromTaggedValue(scene_state, tagged_value, /*parent*/{});
  assert(scene_state.markers().size() == 2);
  assert(scene_state.marker(0).name != scene_state.marker(1).name);
}


int main()
{
  testCreatingABodyFromATaggedValue();
  testCreatingABodyFromATaggedValueWithConflictingMarkerNames();
  testBody();
}
