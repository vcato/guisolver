#include "positionstate.hpp"
#include "transformstate.hpp"


inline Point
  markerPredicted(
    const SceneState &scene_state,
    int marker_index
  )
{
  const SceneState::Marker &marker = scene_state.marker(marker_index);
  const Point &local = makePoint(marker.position);

  Optional<BodyIndex> maybe_body_index = marker.maybe_body_index;

  if (maybe_body_index) {
    BodyIndex body_index = *maybe_body_index;
    const SceneState::Body &body_state = scene_state.body(body_index);
    return makeTransformFromState(body_state.global) * local;
  }
  else {
    return local;
  }
}
