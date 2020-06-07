#include "globaltransform.hpp"

#include "maketransform.hpp"
#include "transformstate.hpp"
#include "positionstate.hpp"
#include "sceneobjects.hpp"
#include "scenetransform.hpp"


Transform
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


Transform
unscaledGlobalTransform(
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


Point
parentPoint(const SceneState::Body &body_state, const Point &local)
{
  return makeScaledTransformFromState(body_state.transform) * local;
}


Transform
parentScaledTransform(
  const SceneState::Body &body_state,
  const Transform &local
)
{
  return makeScaledTransformFromState(body_state.transform) * local;
}


static Point
bodyPointRelativeToScene(
  Point local,
  Optional<BodyIndex> maybe_body_index,
  const SceneState &scene_state
)
{
  while (maybe_body_index) {
    local = parentPoint(scene_state.body(*maybe_body_index), local);
    maybe_body_index = scene_state.body(*maybe_body_index).maybe_parent_index;
  }

  return local;
}


Point
markerPredicted(const SceneState &scene_state, MarkerIndex marker_index)
{
  const SceneState::Marker &marker = scene_state.marker(marker_index);
  Optional<BodyIndex> maybe_body_index = marker.maybe_body_index;
  Point local = makePointFromPositionState(marker.position);
  return bodyPointRelativeToScene(local, maybe_body_index, scene_state);
}


#if ADD_BODY_MESH_POSITION_TO_POINT_LINK
Point
bodyMeshPositionPredicted(
  const SceneState &scene_state,
  BodyMeshPosition body_mesh_position
)
{
  // We'll  need to get the position, apply the meshes's geometry transform,
  // then apply the body hierarchy transform.

  // There should be something like this already for determining the
  // manipulator position.
  Vec3 local = bodyMeshPositionRelativeToBody(body_mesh_position, scene_state);

  return
    bodyPointRelativeToScene(
      makePointFromScenePoint(local),
      body_mesh_position.array.body_mesh.body.index,
      scene_state
    );
}
#endif
