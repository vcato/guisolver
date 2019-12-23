#include "scenestate.hpp"

#include <sstream>
#include "contains.hpp"


using std::ostringstream;

template <typename Name>
static Name nextUnusedName(const vector<Name> &used_names, const Name &prefix)
{
  for (int id = 1;; ++id) {
    ostringstream name_stream;
    name_stream << prefix << id;
    Name next_name_to_try = name_stream.str();

    if (!contains(used_names, next_name_to_try)) {
      return next_name_to_try;
    }
  }
}

static SceneState::Marker::Name namePrefix(bool is_local)
{
  if (is_local) {
    return "local";
  }
  else {
    return "global";
  }
}


vector<SceneState::Marker::Name> markerNames(const SceneState &state)
{
  vector<SceneState::Marker::Name> result;

  for (auto &marker : state.markers()) {
    result.push_back(marker.name);
  }

  return result;
}


static SceneState::Marker::Name
  newMarkerName(const SceneState &state, bool is_local)
{
  return nextUnusedName(markerNames(state), namePrefix(is_local));
}


MarkerIndex
createMarkerInState(
  SceneState &state,
  Optional<BodyIndex> maybe_body_index
)
{
  bool is_local = maybe_body_index.hasValue();
  MarkerIndex index = state.createMarker(newMarkerName(state, is_local));
  state.marker(index).maybe_body_index = maybe_body_index;
  return index;
}


BodyIndex
createBodyInState(SceneState &state, Optional<BodyIndex> maybe_parent_index)
{
  return state.createBody(maybe_parent_index);
}


SceneState::SceneState()
{
}
