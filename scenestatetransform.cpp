#include "scenestatetransform.hpp"

#include "transform.hpp"
#include "transformstate.hpp"
#include "globaltransform.hpp"


void
changeBodyTransformToPreserveGlobal(
  BodyIndex new_body_index,
  SceneState &scene_state,
  Optional<BodyIndex> maybe_old_parent_body_index,
  Optional<BodyIndex> maybe_new_parent_body_index
)
{
  SceneState::Body &body_state = scene_state.body(new_body_index);

  Transform old_body_transform =
    makeScaledTransformFromState(body_state.transform);

  Transform old_parent_global_transform =
    globalTransform(maybe_old_parent_body_index, scene_state);

  Transform new_parent_global_transform =
    globalTransform(maybe_new_parent_body_index, scene_state);

  Transform body_global_transform =
    old_parent_global_transform*old_body_transform;

  Transform new_body_transform =
    new_parent_global_transform.inverse()*body_global_transform;

  body_state.transform = transformState(new_body_transform);
}


void
changeMarkerPositionToPreserveGlobal(
  MarkerIndex marker_index,
  SceneState &scene_state,
  Optional<BodyIndex> maybe_old_parent_body_index,
  Optional<BodyIndex> maybe_new_parent_body_index
)
{
  SceneState::Marker &marker_state = scene_state.marker(marker_index);

  Transform new_parent_global_transform =
    globalTransform(maybe_new_parent_body_index, scene_state);

  Transform old_parent_global_transform =
    globalTransform(maybe_old_parent_body_index, scene_state);

  Point old_marker_position = makePointFromPositionState(marker_state.position);

  Point marker_global_position =
    old_parent_global_transform*old_marker_position;

  Point new_marker_position =
    new_parent_global_transform.inverse()*marker_global_position;

  marker_state.position = makePositionStateFromPoint(new_marker_position);
}


