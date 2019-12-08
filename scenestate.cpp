#include "scenestate.hpp"

#include "contains.hpp"
#include "maketransform.hpp"
#include "transformstate.hpp"

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


MarkerIndex createMarkerInState(SceneState &state, bool is_local)
{
  MarkerIndex index = state.addMarker(newMarkerName(state, is_local));
  state.marker(index).is_local = is_local;
  return index;
}


Point SceneState::markerPredicted(int marker_index) const
{
  if (_markers[marker_index].is_local) {
    return makeTransformFromState(box.global) * _markers[marker_index].position;
  }
  else {
    return _markers[marker_index].position;
  }
}
