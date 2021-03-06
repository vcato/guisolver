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
  scene_state.body(body_index).createBox();
  setAll(scene_state.body(body_index).solve_flags, true);
  return body_index;
}


static BodyIndex createGlobalBodyIn(SceneState &scene_state)
{
  return createBodyIn(scene_state, /*maybe_parent_index*/{});
}


static SceneState::Box &bodyBox(SceneState::Body &body)
{
  assert(body.boxes.size() == 1);
  return body.boxes[0];
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
    "  Line {\n"
    "    start {\n"
    "      x: 1\n"
    "      y: 2.5\n"
    "      z: 3.5\n"
    "    }\n"
    "    end {\n"
    "      x: 1.5\n"
    "      y: 2.5\n"
    "      z: 3.5\n"
    "    }\n"
    "  }\n"
    "  Mesh {\n"
    "    scale {\n"
    "      x: 1.5\n"
    "      y: 2.5\n"
    "      z: 3.5\n"
    "    }\n"
    "    center {\n"
    "      x: 1.25\n"
    "      y: 2.25\n"
    "      z: 3.25\n"
    "    }\n"
    "    positions {\n"
    "      0 {\n"
    "        x: 1.5\n"
    "        y: 2.25\n"
    "        z: 3.75\n"
    "      }\n"
    "      1 {\n"
    "        x: 2.5\n"
    "        y: 2.25\n"
    "        z: 3.75\n"
    "      }\n"
    "      2 {\n"
    "        x: 3.5\n"
    "        y: 2.25\n"
    "        z: 3.75\n"
    "      }\n"
    "      3 {\n"
    "        x: 4.5\n"
    "        y: 2.25\n"
    "        z: 3.75\n"
    "      }\n"
    "    }\n"
    "    Triangle {\n"
    "      vertex1: 0\n"
    "      vertex2: 1\n"
    "      vertex3: 2\n"
    "    }\n"
    "    Triangle {\n"
    "      vertex1: 2\n"
    "      vertex2: 1\n"
    "      vertex3: 3\n"
    "    }\n"
    "  }\n"
    "}\n";

  istringstream stream(text);
  ScanTaggedValueResult result = scanTaggedValueFrom(stream);
  assert(result.isValue());
  SceneState state;
  const TaggedValue &tagged_value = result.asValue();
  MarkerNameMap marker_name_map;

  createBodyFromTaggedValue(
    state,
    tagged_value,
    /*maybe_parent_index*/{},
    marker_name_map
  );

  assert(state.bodies().size() == 1);
  BodyIndex body_index = 0;
  const SceneState::Body &body = state.body(body_index);
  assert(body.boxes.size() == 2);
  const SceneState::Box &box = body.boxes[0];
  assert(box.scale.x == 1);
  assert(box.scale.y == 2.5);
  assert(box.scale.z == 3.5);
  assert(box.center.x == 1.5);
  assert(box.center.y == 2.5);
  assert(box.center.z == 3.5);
  assert(body.solve_flags.translation.x == false);
  assert(body.solve_flags.translation.y == true);
  assert(body.lines.size() == 1);
  assert(body.meshes.size() == 1);
  assert(body.meshes[0].scale.x == 1.5);
  assert(body.meshes[0].center.x == 1.25);
  assert(body.meshes[0].shape.positions.size() == 4);
  assert(body.meshes[0].shape.positions[0].x == 1.5);
  assert(body.meshes[0].shape.triangles.size() == 2);
  assert(body.meshes[0].shape.triangles[1].v3 == 3);

  TaggedValue parent_tagged_value("");

  createBodyTaggedValue(
    parent_tagged_value,
    body_index,
    state
  );

  SceneState new_scene_state;
  MarkerNameMap new_marker_name_map;

  const TaggedValue *transform_tagged_value_ptr =
    findChild(parent_tagged_value, "Transform");

  assert(transform_tagged_value_ptr);

  BodyIndex new_body_index =
    createBodyFromTaggedValue(
      new_scene_state,
      *transform_tagged_value_ptr,
      /*maybe_parent_index*/{},
      new_marker_name_map
    );

  SceneState::Body &new_body_state = new_scene_state.body(new_body_index);
  assert(new_body_state.meshes.size() == 1);
  const SceneState::Mesh &new_mesh_state = new_body_state.meshes[0];
  assert(new_mesh_state.shape.positions.size() == 4);
  assert(new_mesh_state.shape.positions[0].x == 1.5);
  assert(new_mesh_state.shape.triangles.size() == 2);
  assert(new_mesh_state.scale.x == 1.5);
  assert(new_mesh_state.center.x == 1.25);
}


static void testBody()
{
  Optional<TaggedValue> maybe_transform_tagged_value;

  // Create a tagged value
  {
    SceneState scene_state;

    BodyIndex body_index = createGlobalBodyIn(scene_state);
    scene_state.body(body_index).name = "testbody";
    scene_state.body(body_index).solve_flags.translation.y = false;
    scene_state.body(body_index).expressions.scale = "var1";
    bodyBox(scene_state.body(body_index)).center.y = 2.5;

    TaggedValue tagged_value("root");
    createBodyTaggedValue(tagged_value, body_index, scene_state);

    const TaggedValue *transform_tagged_value_ptr =
      findChild(tagged_value, "Transform");

    assert(transform_tagged_value_ptr);
    maybe_transform_tagged_value = *transform_tagged_value_ptr;
  }

  // Create a body from the tagged value.
  {
    SceneState scene_state;
    MarkerNameMap marker_name_map;

    BodyIndex body_index =
      createBodyFromTaggedValue(
        scene_state,
        *maybe_transform_tagged_value,
        /*maybe_parent_index*/{},
        marker_name_map
      );

    assert(scene_state.body(body_index).solve_flags.translation.y == false);
    assert(bodyBox(scene_state.body(body_index)).center.y == 2.5);
    assert(scene_state.body(body_index).expressions.scale == "var1");
    assert(scene_state.body(body_index).name == "testbody");
  }
}


static void testCreatingABodyFromATaggedValueWithConflictingNames()
{
  SceneState scene_state;
  BodyIndex body1_index = scene_state.createBody();
  scene_state.createMarker(body1_index);
  BodyIndex body2_index = scene_state.createBody(body1_index);
  scene_state.createMarker(body2_index);

  {
    TaggedValue root_tag_value("");
    createBodyTaggedValue(root_tag_value, body1_index, scene_state);
    const TaggedValue &tagged_value = root_tag_value.children[0];
    MarkerNameMap marker_name_map;

    createBodyFromTaggedValue(
      scene_state, tagged_value, /*parent*/{}, marker_name_map
    );

    assert(marker_name_map.size() == 2);
  }

  assert(scene_state.markers().size() == 4);
  assert(scene_state.marker(0).name != scene_state.marker(2).name);
  assert(scene_state.bodies().size() == 4);
  assert(scene_state.body(0).name != scene_state.body(2).name);
}


int main()
{
  testCreatingABodyFromATaggedValue();
  testCreatingABodyFromATaggedValueWithConflictingNames();
  testBody();
}
