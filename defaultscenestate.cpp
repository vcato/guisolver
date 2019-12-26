#include "defaultscenestate.hpp"

#include "scene.hpp"
#include "maketransform.hpp"
#include "contains.hpp"
#include "rotationvector.hpp"
#include "transformstate.hpp"

using std::ostringstream;

static void
  createDistanceError(
    SceneState &state,
    MarkerIndex local_marker_index,
    MarkerIndex global_marker_index
  )
{
  SceneState::DistanceError &new_distance_error = state.createDistanceError();
  new_distance_error.optional_start_marker_index = local_marker_index;
  new_distance_error.optional_end_marker_index = global_marker_index;
}


static void
  createMarker(
    SceneState &state,
    const MarkerPosition &position,
    Optional<BodyIndex> maybe_body_index
  )
{
  MarkerIndex index = createMarkerInState(state, maybe_body_index);
  state.marker(index).position = position;
}


static void
  createLocalMarker(
    SceneState &state,
    const MarkerPosition &position,
    BodyIndex body_index
  )
{
  createMarker(state, position, body_index);
}


static void
  createGlobalMarker(SceneState &state, const MarkerPosition &position)
{
  createMarker(state,position, /*maybe_body_index*/{});
}


static Point defaultBoxTranslation()
{
  return {0,1,0};
}


static CoordinateAxes defaultBoxCoordinateAxes()
{
  return
    CoordinateAxes{
      Scene::Vector(0,0,-1),
      Scene::Vector(0,1, 0),
      Scene::Vector(1,0, 0)
    };
}


SceneState defaultSceneState()
{
  SceneState result;

  BodyIndex body_index =
    createBodyInState(
      result,
      /*maybe_parent_index*/{},
      SceneState::defaultBodyScale()
    );

  Transform default_box_transform =
    makeTransform(defaultBoxCoordinateAxes(), defaultBoxTranslation());

  result.body(body_index).transform = transformState(default_box_transform);

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
