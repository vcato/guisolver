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
  state.body(body_index).addBox();
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


static void testWith(const SceneState &state)
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
  testWith(state);
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
    "      x: 0\n"
    "      y: 0\n"
    "      z: 0\n"
    "    }\n"
    "    rotation {\n"
    "      x: 0\n"
    "      y: 0\n"
    "      z: 0\n"
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
    "        x: 0\n"
    "        y: 0\n"
    "        z: 0\n"
    "      }\n"
    "      rotation {\n"
    "        x: 0\n"
    "        y: 0\n"
    "        z: 0\n"
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
  testWith(state);
}


static void testWithMultipleTransforms()
{
  string expected_string =
    "Scene {\n"
    "  Transform {\n"
    "    name: \"body1\"\n"
    "    translation {\n"
    "      x: 0\n"
    "      y: 0\n"
    "      z: 0\n"
    "    }\n"
    "    rotation {\n"
    "      x: 0\n"
    "      y: 0\n"
    "      z: 0\n"
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
    "  Transform {\n"
    "    name: \"body2\"\n"
    "    translation {\n"
    "      x: 0\n"
    "      y: 0\n"
    "      z: 0\n"
    "    }\n"
    "    rotation {\n"
    "      x: 0\n"
    "      y: 0\n"
    "      z: 0\n"
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
  BodyIndex box1_index = createBodyInState(state, /*maybe_parent_index*/{});
  setAll(state.body(box1_index).solve_flags, true);
  bodyBox(state.body(box1_index)).scale = box_scale;
  BodyIndex box2_index = createBodyInState(state, /*maybe_parent_index*/{});
  setAll(state.body(box2_index).solve_flags, true);
  bodyBox(state.body(box2_index)).scale = box_scale;
  string state_string = sceneStateString(state);
  assert(state_string == expected_string);
  testWith(state);
}


int main()
{
  testWith(defaultSceneState());
  testWithAdditionalDistanceError();
  testWithChildTransform();
  testWithMultipleTransforms();
}
