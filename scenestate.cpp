#include "scenestate.hpp"

#include <sstream>
#include "contains.hpp"
#include "indicesof.hpp"


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


MarkerIndex SceneState::createMarker(Optional<BodyIndex> maybe_body_index)
{
  bool is_local = maybe_body_index.hasValue();
  SceneState::Marker::Name name = newMarkerName(*this, is_local);
  MarkerIndex index = createMarker(name);
  marker(index).maybe_body_index = maybe_body_index;
  return index;
}


MarkerIndex SceneState::duplicateMarker(MarkerIndex from_marker_index)
{
  bool is_local = _markers[from_marker_index].maybe_body_index.hasValue();
  MarkerIndex new_marker_index = _markers.size();
  _markers.push_back(_markers[from_marker_index]);
  _markers[new_marker_index].name = newMarkerName(*this, is_local);
  return new_marker_index;
}


BodyIndex
createBodyInState(SceneState &state, Optional<BodyIndex> maybe_parent_index)
{
  return state.createBody(maybe_parent_index);
}


SceneState::SceneState()
{
}


bool SceneState::bodyHasChildren(BodyIndex body_index) const
{
  const SceneState &state = *this;

  for (auto other_body_index : indicesOf(state.bodies())) {
    if (state.body(other_body_index).maybe_parent_index == body_index) {
      return true;
    }
  }

  for (auto marker_index : indicesOf(state.markers())) {
    if (state.marker(marker_index).maybe_body_index == body_index) {
      return true;
    }
  }

  return false;
}


BodyIndex
SceneState::createBody(Optional<BodyIndex> maybe_parent_index)
{
  BodyIndex new_index = _bodies.size();
  _bodies.emplace_back();
  _bodies.back().maybe_parent_index = maybe_parent_index;
  return new_index;
}
