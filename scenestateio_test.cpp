#include "scenestateio.hpp"

#include <sstream>
#include "defaultscenestate.hpp"

#define ADD_TEST 0

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
  createBodyInState(state, /*maybe_parent_index*/{});
  SceneState::DistanceError &distance_error = state.addDistanceError();
  distance_error.weight = 2.5;
  distance_error.desired_distance = 3.5;
  testWith(state);
}


#if ADD_TEST
static void testWithAdditionalTransform()
{
  SceneState state;
  BodyIndex body1_index = createBodyInState(state, /*maybe_parent_index*/{});
  createBodyInState(state, /*maybe_parent_index*/body1_index);
  testWith(state);
}
#endif


int main()
{
  testWith(defaultSceneState());
  testWithAdditionalDistanceError();
#if ADD_TEST
  testWithAdditionalTransform();
#endif
}
