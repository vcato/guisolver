#include "scenestate.hpp"

#include <sstream>
#include "contains.hpp"
#include "indicesof.hpp"
#include "removeindexfrom.hpp"
#include "nextunusedname.hpp"

using std::ostringstream;
using std::cerr;
using std::ostream;


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


static vector<SceneState::Variable::Name> variableNames(const SceneState &state)
{
  vector<SceneState::Variable::Name> result;

  for (auto &variable : state.variables) {
    result.push_back(variable.name);
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

  for (auto i : indicesOf(scene_state.markers())) {
    if (scene_state.marker(i).maybe_body_index == maybe_body_index) {
      result.push_back(i);
    }
  }

  return result;
}


vector<DistanceErrorIndex>
indicesOfDistanceErrorsOnBody(
  Optional<BodyIndex> maybe_body_index,
  const SceneState &scene_state
)
{
  vector<DistanceErrorIndex> result;

  for (auto i : indicesOf(scene_state.distance_errors)) {
    if (scene_state.distance_errors[i].maybe_body_index == maybe_body_index) {
      result.push_back(i);
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
  findMarkerWithName(
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
  findBodyWithName(
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


extern Optional<VariableIndex>
findVariableWithName(
  const SceneState &scene_state,
  const SceneState::Variable::Name &name
)
{
  for (auto i : indicesOf(scene_state.variables)) {
    if (scene_state.variables[i].name == name) {
      return i;
    }
  }

  return {};
}


void SceneState::removeMarker(MarkerIndex index_to_remove)
{
  removeIndexFrom(_markers, index_to_remove);

  for (auto &distance_error : distance_errors) {
    _handleMarkerRemoved(distance_error.optional_start, index_to_remove);
    _handleMarkerRemoved(distance_error.optional_end, index_to_remove);
  }
}


void SceneState::removeDistanceError(DistanceErrorIndex index)
{
  removeIndexFrom(distance_errors, index);
}


void SceneState::removeVariable(VariableIndex index)
{
  removeIndexFrom(variables, index);
}


VariableIndex SceneState::createVariable()
{
  Variable::Name name = nextUnusedName(variableNames(*this), "var");
  VariableIndex index = variables.size();
  variables.push_back(Variable());
  variables[index].name = name;
  return index;
}


static Expression &
expression(const BodyTranslationComponent &element, SceneState &scene_state)
{
  SceneState::XYZExpressions &translation_expressions =
    scene_state
    .body(bodyOf(element).index)
    .expressions
    .translation;

  return xyzExpressionsComponent(translation_expressions, element.component);
}


static Expression &
expression(const BodyRotationComponent &element, SceneState &scene_state)
{
  SceneState::XYZExpressions &translation_expressions =
    scene_state
    .body(bodyOf(element).index)
    .expressions
    .rotation;

  return xyzExpressionsComponent(translation_expressions, element.component);
}


static Expression &
expression(const BodyScale &element, SceneState &scene_state)
{
  return
    scene_state
    .body(element.body.index)
    .expressions
    .scale;
}


static Expression &
expression(const BodyBoxScaleComponent &element, SceneState &scene_state)
{
  return
    xyzExpressionsComponent(
      scene_state
      .body(bodyOf(element).index)
      .boxes[bodyBoxOf(element).index]
      .scale_expressions,
      element.component
    );
}


static Expression &
expression(const BodyBoxCenterComponent &element, SceneState &scene_state)
{
  return
    xyzExpressionsComponent(
      scene_state
      .body(bodyOf(element).index)
      .boxes[bodyBoxOf(element).index]
      .center_expressions,
      element.component
    );
}


static Expression &
expression(const MarkerPositionComponent &element, SceneState &scene_state)
{
  return
    xyzExpressionsComponent(
      scene_state
      .marker(markerOf(element).index)
      .position_expressions,
      element.component
    );
}


SceneState::Expression &
channelExpression(const Channel &channel, SceneState &scene_state)
{
  SceneState::Expression *expression_ptr = nullptr;

  channel.visit([&](auto &channel){
    expression_ptr = &expression(channel, scene_state);
  });

  assert(expression_ptr);
  return *expression_ptr;
}


MeshIndex SceneState::Body::createMesh(const MeshShape &mesh_shape)
{
  MeshIndex mesh_index = meshes.size();
  meshes.emplace_back();
  meshes.back().shape = mesh_shape;
  return mesh_index;
}


