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
    return makeTransformFromState(boxBodyState(scene_state).global) * local;
  }
  else {
    return local;
  }
}
