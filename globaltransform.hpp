#include "point.hpp"
#include "transform.hpp"
#include "scenestate.hpp"


// This transform a point from a body's coordinate system to the parent's
// coordinate system.
extern Point
  parentPoint(const SceneState::Body &body_state, const Point &local);

extern Transform
  parentScaledTransform(
    const SceneState::Body &body_state,
    const Transform &local
  );

extern Transform
  parentUnscaledTransform(
    const SceneState::Body &body_state,
    Transform local
  );

extern Point
  markerPredicted(const SceneState &scene_state, MarkerIndex marker_index);

#if ADD_BODY_MESH_POSITION_TO_POINT_LINK
extern Point
  bodyMeshPositionPredicted(const SceneState &scene_state, BodyMeshPosition);
#endif

extern Transform
  unscaledGlobalTransform(
    Optional<BodyIndex> maybe_body_index,
    const SceneState &scene_state
  );


inline Transform
scaledGlobalTransform(
  Optional<BodyIndex> maybe_body_index,
  const SceneState &scene_state
)
{
  Transform transform = Transform::Identity();

  while (maybe_body_index) {
    const SceneState::Body &body_state = scene_state.body(*maybe_body_index);
    transform = parentScaledTransform(body_state, transform);
    maybe_body_index = scene_state.body(*maybe_body_index).maybe_parent_index;
  }

  return transform;
}
