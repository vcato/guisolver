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


static void test2()
{
  SceneState state;
  SceneState::DistanceError &distance_error = state.addDistanceError();
  distance_error.weight = 2.5;
  distance_error.desired_distance = 3.5;
  testWith(state);
}


int main()
{
  testWith(defaultSceneState());
  test2();
}
