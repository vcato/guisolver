#include "scenestate.hpp"

#include <sstream>
#include "contains.hpp"
#include "indicesof.hpp"

using std::ostringstream;
using std::cerr;


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


vector<SceneState::Body::Name> bodyNames(const SceneState &state)
{
  vector<SceneState::Body::Name> result;

  for (auto &body : state.bodies()) {
    result.push_back(body.name);
  }

  return result;
}


static SceneState::Marker::Name
  newMarkerName(const SceneState &state, bool is_local)
{
  return nextUnusedName(markerNames(state), namePrefix(is_local));
}


static SceneState::Body::Name newBodyName(const SceneState &state)
{
  return nextUnusedName(bodyNames(state), SceneState::Body::Name("body"));
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


SceneState::SceneState()
{
}


vector<MarkerIndex>
indicesOfMarkersOnBody(
  Optional<BodyIndex> maybe_body_index,
  const SceneState &scene_state
)
{
  vector<MarkerIndex> result;

  for (auto marker_index : indicesOf(scene_state.markers())) {
    if (scene_state.marker(marker_index).maybe_body_index == maybe_body_index) {
      result.push_back(marker_index);
    }
  }

  return result;
}


vector<BodyIndex>
indicesOfChildBodies(
  Optional<BodyIndex> maybe_body_index,
  const SceneState &scene_state
)
{
  vector<BodyIndex> result;

  for (auto other_body_index : indicesOf(scene_state.bodies())) {
    bool is_a_child =
      scene_state.body(other_body_index).maybe_parent_index == maybe_body_index;

    if (is_a_child) {
      result.push_back(other_body_index);
    }
  }

  return result;
}


bool SceneState::bodyHasChildren(BodyIndex body_index) const
{
  const SceneState &state = *this;

  for (auto other_body_index : indicesOf(state.bodies())) {
    if (state.body(other_body_index).maybe_parent_index == body_index) {
      return true;
    }
  }

  if (!indicesOfMarkersOnBody(body_index, *this).empty()) {
    return true;
  }

  return false;
}


BodyIndex
SceneState::createBody(Optional<BodyIndex> maybe_parent_index)
{
  BodyIndex new_index = _bodies.size();
  _bodies.emplace_back(newBodyName(*this));
  body(new_index).maybe_parent_index = maybe_parent_index;
  return new_index;
}


void SceneState::removeBody(BodyIndex index_to_remove)
{
  assert(!bodyHasChildren(index_to_remove));
  removeIndexFrom(_bodies, index_to_remove);

  for (auto marker_index : indicesOf(_markers)) {
    if (_markers[marker_index].maybe_body_index) {
      if (*_markers[marker_index].maybe_body_index >= index_to_remove) {
        --*_markers[marker_index].maybe_body_index;
      }
    }
  }
}


bool
hasAncestor(
  BodyIndex body_index,
  BodyIndex ancestor_body_index,
  const SceneState &state
)
{
  if (body_index == ancestor_body_index) {
    return true;
  }

  Optional<BodyIndex> maybe_parent_body_index =
    state.body(body_index).maybe_parent_index;

  if (!maybe_parent_body_index) {
    return false;
  }

  return hasAncestor(*maybe_parent_body_index, ancestor_body_index, state);
}


Optional<MarkerIndex>
  findMarkerIndex(
    const SceneState &scene_state,
    const SceneState::Marker::Name &name
  )
{
  for (auto i : indicesOf(scene_state.markers())) {
    if (scene_state.marker(i).name == name) {
      return i;
    }
  }

  return {};
}


Optional<BodyIndex>
  findBodyIndex(
    const SceneState &scene_state,
    const SceneState::Body::Name &name
  )
{
  for (auto i : indicesOf(scene_state.bodies())) {
    if (scene_state.body(i).name == name) {
      return i;
    }
  }

  return {};
}
