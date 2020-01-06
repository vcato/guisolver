#include "defaultscenestate.hpp"

using std::ostringstream;
using std::cerr;

static void
  createDistanceError(
    SceneState &state,
    MarkerIndex local_marker_index,
    MarkerIndex global_marker_index
  )
{
  DistanceErrorIndex index = state.createDistanceError();
  SceneState::DistanceError &new_distance_error = state.distance_errors[index];
  new_distance_error.optional_start_marker_index = local_marker_index;
  new_distance_error.optional_end_marker_index = global_marker_index;
}


static void
  createMarker(
    SceneState &state,
    const PositionState &position,
    Optional<BodyIndex> maybe_body_index
  )
{
  MarkerIndex index = state.createMarker(maybe_body_index);
  state.marker(index).position = position;
}


static void
  createLocalMarker(
    SceneState &state,
    const PositionState &position,
    BodyIndex body_index
  )
{
  createMarker(state, position, body_index);
}


static void
  createGlobalMarker(SceneState &state, const PositionState &position)
{
  createMarker(state,position, /*maybe_body_index*/{});
}


SceneState defaultSceneState()
{
  SceneState result;
  SceneState::XYZ scale = { 5.0, 0.1, 10.0 };
  BodyIndex body_index = result.createBody(/*maybe_parent_index*/{});
  result.body(body_index).addBox();
  setAll(result.body(body_index).solve_flags, true);
  result.body(body_index).boxes[0].scale = scale;

  result.body(body_index).transform.rotation = {0, 90, 0};
  result.body(body_index).transform.translation = {0, 1, 0};

  createLocalMarker(result,  {1,1,0}, body_index);
  createLocalMarker(result,  {1,1,1}, body_index);
  createLocalMarker(result,  {0,1,1}, body_index);
  createGlobalMarker(result, {1,2,0});
  createGlobalMarker(result, {1,2,1});
  createGlobalMarker(result, {0,2,1});

  int n_lines = 3;

  for (int i=0; i!=n_lines; ++i) {
    MarkerIndex local_marker_index = i;
    MarkerIndex global_marker_index = i + 3;

    createDistanceError(
      result,
      local_marker_index,
      global_marker_index
    );
  }

  return result;
}
