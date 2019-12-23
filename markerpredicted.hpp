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

  if (marker.is_local) {
    BodyIndex body_index = boxBodyIndex();
    const SceneState::Body &body_state = scene_state.body(body_index);
    return makeTransformFromState(body_state.global) * local;
  }
  else {
    return local;
  }
}
