#include "scenestateio.hpp"

#include <sstream>
#include "defaultscenestate.hpp"

using std::istringstream;
using std::ostringstream;
using std::cerr;
using std::string;


static BodyIndex
createBodyInState(SceneState &state, Optional<BodyIndex> maybe_parent_index)
{
  BodyIndex body_index = state.createBody(maybe_parent_index);
  state.body(body_index).createBox();
  return body_index;
}


static string sceneStateString(const SceneState &state)
{
  ostringstream output_stream;
  printSceneStateOn(output_stream, state);
  return output_stream.str();
}


static SceneState::Box &bodyBox(SceneState::Body &body)
{
  assert(body.boxes.size() == 1);
  return body.boxes[0];
}


static void testRescanWith(const SceneState &state)
{
  std::string state_string = sceneStateString(state);
  istringstream input_stream(state_string);
  Expected<SceneState> scan_result = scanSceneStateFrom(input_stream);
  string new_state_string = sceneStateString(scan_result.asValue());

  if (new_state_string != state_string) {
    cerr << "state_string:\n";
    cerr << state_string << "\n";
    cerr << "new_state_string:\n";
    cerr << new_state_string << "\n";
  }

  assert(new_state_string == state_string);
}


static void testWithAdditionalDistanceError()
{
  SceneState state;
  BodyIndex body_index = createBodyInState(state, /*maybe_parent_index*/{});
  setAll(state.body(body_index).solve_flags, true);
  DistanceErrorIndex index = state.createDistanceError();
  SceneState::DistanceError &distance_error = state.distance_errors[index];
  distance_error.weight = 2.5;
  distance_error.desired_distance = 3.5;
  testRescanWith(state);
}


static void testWithChildTransform()
{
  SceneState state;
  SceneState::XYZ scale = { 5, 0.1, 10 };
  BodyIndex body1_index = createBodyInState(state, /*parent*/{});
  setAll(state.body(body1_index).solve_flags, true);
  bodyBox(state.body(body1_index)).scale = scale;
  BodyIndex body2_index = createBodyInState(state, /*parent*/body1_index);
  setAll(state.body(body2_index).solve_flags, true);
  bodyBox(state.body(body2_index)).scale = scale;
  string state_string = sceneStateString(state);

  string expected_string =
    "Scene {\n"
    "  Transform {\n"
    "    name: \"body1\"\n"
    "    translation {\n"
    "      x: 0 {\n"
    "        solve: true\n"
    "      }\n"
    "      y: 0 {\n"
    "        solve: true\n"
    "      }\n"
    "      z: 0 {\n"
    "        solve: true\n"
    "      }\n"
    "    }\n"
    "    rotation {\n"
    "      x: 0 {\n"
    "        solve: true\n"
    "      }\n"
    "      y: 0 {\n"
    "        solve: true\n"
    "      }\n"
    "      z: 0 {\n"
    "        solve: true\n"
    "      }\n"
    "    }\n"
    "    Box {\n"
    "      scale {\n"
    "        x: 5\n"
    "        y: 0.1\n"
    "        z: 10\n"
    "      }\n"
    "      center {\n"
    "        x: 0\n"
    "        y: 0\n"
    "        z: 0\n"
    "      }\n"
    "    }\n"
    "    Transform {\n"
    "      name: \"body2\"\n"
    "      translation {\n"
    "        x: 0 {\n"
    "          solve: true\n"
    "        }\n"
    "        y: 0 {\n"
    "          solve: true\n"
    "        }\n"
    "        z: 0 {\n"
    "          solve: true\n"
    "        }\n"
    "      }\n"
    "      rotation {\n"
    "        x: 0 {\n"
    "          solve: true\n"
    "        }\n"
    "        y: 0 {\n"
    "          solve: true\n"
    "        }\n"
    "        z: 0 {\n"
    "          solve: true\n"
    "        }\n"
    "      }\n"
    "      Box {\n"
    "        scale {\n"
    "          x: 5\n"
    "          y: 0.1\n"
    "          z: 10\n"
    "        }\n"
    "        center {\n"
    "          x: 0\n"
    "          y: 0\n"
    "          z: 0\n"
    "        }\n"
    "      }\n"
    "    }\n"
    "  }\n"
    "}\n";

  assert(state_string == expected_string);
  testRescanWith(state);
}


static void testWithMultipleTransforms()
{
  string expected_string =
    "Scene {\n"
    "  Transform {\n"
    "    name: \"body1\"\n"
    "    translation {\n"
    "      x: 0 {\n"
    "        solve: true\n"
    "      }\n"
    "      y: 0 {\n"
    "        solve: true\n"
    "      }\n"
    "      z: 0 {\n"
    "        solve: true\n"
    "      }\n"
    "    }\n"
    "    rotation {\n"
    "      x: 0 {\n"
    "        solve: true\n"
    "      }\n"
    "      y: 0 {\n"
    "        solve: true\n"
    "      }\n"
    "      z: 0 {\n"
    "        solve: true\n"
    "      }\n"
    "    }\n"
    "    Box {\n"
    "      scale {\n"
    "        x: 5\n"
    "        y: 0.1\n"
    "        z: 10\n"
    "      }\n"
    "      center {\n"
    "        x: 0\n"
    "        y: 0\n"
    "        z: 0\n"
    "      }\n"
    "    }\n"
    "    Transform {\n"
    "      name: \"body3\"\n"
    "      translation {\n"
    "        x: 0 {\n"
    "          solve: false\n"
    "        }\n"
    "        y: 0 {\n"
    "          solve: false\n"
    "        }\n"
    "        z: 0 {\n"
    "          solve: false\n"
    "        }\n"
    "      }\n"
    "      rotation {\n"
    "        x: 0 {\n"
    "          solve: false\n"
    "        }\n"
    "        y: 0 {\n"
    "          solve: false\n"
    "        }\n"
    "        z: 0 {\n"
    "          solve: false\n"
    "        }\n"
    "      }\n"
    "      DistanceError {\n"
    "        desired_distance: 0\n"
    "        weight: 1\n"
    "      }\n"
    "    }\n"
    "  }\n"
    "  Transform {\n"
    "    name: \"body2\"\n"
    "    translation {\n"
    "      x: 0 {\n"
    "        solve: true\n"
    "      }\n"
    "      y: 0 {\n"
    "        solve: true\n"
    "      }\n"
    "      z: 0 {\n"
    "        solve: true\n"
    "      }\n"
    "    }\n"
    "    rotation {\n"
    "      x: 0 {\n"
    "        solve: true\n"
    "      }\n"
    "      y: 0 {\n"
    "        solve: true\n"
    "      }\n"
    "      z: 0 {\n"
    "        solve: true\n"
    "      }\n"
    "    }\n"
    "    Box {\n"
    "      scale {\n"
    "        x: 5\n"
    "        y: 0.1\n"
    "        z: 10\n"
    "      }\n"
    "      center {\n"
    "        x: 0\n"
    "        y: 0\n"
    "        z: 0\n"
    "      }\n"
    "    }\n"
    "  }\n"
    "}\n";

  SceneState state;
  SceneState::XYZ box_scale = {5, 0.1, 10};
  BodyIndex body1_index = createBodyInState(state, /*maybe_parent_index*/{});
  setAll(state.body(body1_index).solve_flags, true);
  bodyBox(state.body(body1_index)).scale = box_scale;
  BodyIndex body2_index = createBodyInState(state, /*maybe_parent_index*/{});
  setAll(state.body(body2_index).solve_flags, true);
  bodyBox(state.body(body2_index)).scale = box_scale;
  BodyIndex body3_index = state.createBody(/*parent*/body1_index);
  state.createDistanceError(body3_index);
  string state_string = sceneStateString(state);
  assert(state_string == expected_string);
  testRescanWith(state);
}


static void testWithVariable()
{
  string expected_string =
    "Scene {\n"
    "  Variable {\n"
    "    name: \"var1\"\n"
    "    value: 5.5\n"
    "  }\n"
    "}\n";

  SceneState state;
  VariableIndex variable_index = state.createVariable();
  state.variables[variable_index].name = "var1";
  state.variables[variable_index].value = 5.5;
  string state_string = sceneStateString(state);
  assert(state_string == expected_string);
  testRescanWith(state);
}


static void testWithExpression()
{
  string expected_string =
    "Scene {\n"
    "  Marker {\n"
    "    name: \"global1\"\n"
    "    position {\n"
    "      x: 0 {\n"
    "        expression: \"1+2\"\n"
    "      }\n"
    "      y: 0\n"
    "      z: 0\n"
    "    }\n"
    "  }\n"
    "}\n";

  SceneState state;
  MarkerIndex marker_index = state.createMarker();
  state.marker(marker_index).position_expressions.x = "1+2";
  assert(sceneStateString(state) == expected_string);
  testRescanWith(state);
}


static void testWithBodyMeshPositionRef()
{
  SceneState state;
  BodyIndex body_index = state.createBody();
  SceneState::MeshShape shape;
  MeshIndex mesh_index = state.body(body_index).createMesh(shape);
  DistanceErrorIndex distance_error_index = state.createDistanceError();
  MeshPositionIndex mesh_position_index = 0;

  state.distance_errors[distance_error_index]
    .setStart(Body(body_index).mesh(mesh_index).position(mesh_position_index));

  testRescanWith(state);
}


int main()
{
  testRescanWith(defaultSceneState());
  testWithAdditionalDistanceError();
  testWithChildTransform();
  testWithMultipleTransforms();
  testWithVariable();
  testWithExpression();
  testWithBodyMeshPositionRef();
}
