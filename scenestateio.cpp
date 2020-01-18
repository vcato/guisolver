#include "scenestateio.hpp"

#include <fstream>
#include "scenestatetaggedvalue.hpp"
#include "taggedvalueio.hpp"

using std::ostream;
using std::ofstream;
using std::ifstream;
using std::string;


void printSceneStateOn(ostream &stream, const SceneState &state)
{
  printTaggedValueOn(stream, makeTaggedValueForSceneState(state), /*indent*/0);
}


Expected<SceneState> scanSceneStateFrom(std::istream &stream)
{
  Expected<TaggedValue> expected_tagged_value = scanTaggedValueFrom(stream);

  if (expected_tagged_value.isError()) {
    return expected_tagged_value.asError();
  }

  return makeSceneStateFromTaggedValue(expected_tagged_value.asValue());
}


void saveScene(const SceneState &scene_state, const string &path)
{
  ofstream stream(path);

  if (!stream) {
    assert(false); // not implemented
  }

  printSceneStateOn(stream, scene_state);

  if (!stream) {
    assert(false); // not implemented
  }
}


void loadScene(SceneState &scene_state, const string &path)
{
  ifstream stream(path);

  if (!stream) {
    assert(false); // not implemented
  }

  Expected<SceneState> scan_result = scanSceneStateFrom(stream);

  if (scan_result.isError()) {
    assert(false); // not implemented
  }

  scene_state = scan_result.asValue();
}
