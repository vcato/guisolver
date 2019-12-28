#include "scenestateio.hpp"

#include "scenestatetaggedvalue.hpp"
#include "taggedvalueio.hpp"

using std::ostream;


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
