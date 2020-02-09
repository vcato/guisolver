#include "positionstate.hpp"
#include "transformstate.hpp"


// This transform a point from a body's coordinate system to the parent's
// coordinate system.
inline Point
parentPoint(const SceneState::Body &body_state, const Point &local)
{
  return makeScaledTransformFromState(body_state.transform) * local;
}

inline Transform
parentScaledTransform(
  const SceneState::Body &body_state,
  const Transform &local
)
{
  return makeScaledTransformFromState(body_state.transform) * local;
}


inline Transform
parentUnscaledTransform(
  const SceneState::Body &body_state,
  Transform local
)
{
  setTransformTranslation(
    local, transformTranslation(local) * body_state.transform.scale
  );

  return
    makeUnscaledTransformFromState(body_state.transform, /*scale*/1) * local;
}


inline Point
markerPredicted(const SceneState &scene_state, MarkerIndex marker_index)
{
  const SceneState::Marker &marker = scene_state.marker(marker_index);
  Optional<BodyIndex> maybe_body_index = marker.maybe_body_index;
  Point local = makePointFromPositionState(marker.position);

  while (maybe_body_index) {
    local = parentPoint(scene_state.body(*maybe_body_index), local);
    maybe_body_index = scene_state.body(*maybe_body_index).maybe_parent_index;
  }

  return local;
}


inline Transform
globalTransform(
  Optional<BodyIndex> maybe_body_index,
  const SceneState &scene_state
)
{
  Transform transform = Transform::Identity();

  while (maybe_body_index) {
    const SceneState::Body &body_state = scene_state.body(*maybe_body_index);
    transform = parentUnscaledTransform(body_state, transform);
    maybe_body_index = scene_state.body(*maybe_body_index).maybe_parent_index;
  }

  return transform;
}
