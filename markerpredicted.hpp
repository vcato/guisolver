#include "positionstate.hpp"
#include "transformstate.hpp"


inline Point
  globalizedPoint(const SceneState::Body &body_state, const Point &local)
{
  return makeTransformFromState(body_state.transform) * local;
}


inline Point markerPredicted(const SceneState &scene_state, int marker_index)
{
  const SceneState::Marker &marker = scene_state.marker(marker_index);
  Optional<BodyIndex> maybe_body_index = marker.maybe_body_index;

  Point local = makePoint(marker.position);

  while (maybe_body_index) {
    local = globalizedPoint(scene_state.body(*maybe_body_index), local);
    maybe_body_index = scene_state.body(*maybe_body_index).maybe_parent_index;
  }

  return local;
}
