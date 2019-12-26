#include "scenestateio.hpp"

#include <sstream>
#include "defaultscenestate.hpp"

using std::istringstream;
using std::ostringstream;
using std::cerr;
using std::string;



static string sceneStateString(const SceneState &state)
{
  ostringstream output_stream;
  printSceneStateOn(output_stream, state);
  return output_stream.str();
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
  createBodyInState(state, /*maybe_parent_index*/{}, /*scale*/{1,1,1});
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

  BodyIndex body1_index =
    createBodyInState(state, /*maybe_parent_index*/{}, scale);

  createBodyInState(state, /*maybe_parent_index*/body1_index, scale);
  string state_string = sceneStateString(state);

  string expected_string =
    "Scene {\n"
    "  Transform {\n"
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
    "      scale_x: 5\n"
    "      scale_y: 0.1\n"
    "      scale_z: 10\n"
    "    }\n"
    "    Transform {\n"
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
    "        scale_x: 5\n"
    "        scale_y: 0.1\n"
    "        scale_z: 10\n"
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
    "      scale_x: 5\n"
    "      scale_y: 0.1\n"
    "      scale_z: 10\n"
    "    }\n"
    "  }\n"
    "  Transform {\n"
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
    "      scale_x: 5\n"
    "      scale_y: 0.1\n"
    "      scale_z: 10\n"
    "    }\n"
    "  }\n"
    "}\n";

  SceneState state;
  SceneState::XYZ box_scale = {5, 0.1, 10};
  createBodyInState(state, /*maybe_parent_index*/{}, box_scale);
  createBodyInState(state, /*maybe_parent_index*/{}, box_scale);
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
