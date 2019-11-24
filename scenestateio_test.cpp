#include "scenestateio.hpp"

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


int main()
{
  std::string state_string = sceneStateString(defaultSceneState());
  istringstream input_stream(state_string);
#if ADD_TEST
  cerr << "---1\n";
  Expected<SceneState> scan_result = scanSceneStateFrom(input_stream);
  cerr << "---2\n";
  string new_state_string = sceneStateString(scan_result.asValue());
  cerr << "---3\n";

  if (new_state_string != state_string) {
    cerr << "state_string:\n";
    //cerr << state_string << "\n";
    cerr << "new_state_string:\n";
    //cerr << new_state_string << "\n";
  }

  assert(new_state_string == state_string);
#endif
}
