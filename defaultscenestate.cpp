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
  SceneState::DistanceError &new_distance_error = state.addDistanceError();
  new_distance_error.optional_start_marker_index = local_marker_index;
  new_distance_error.optional_end_marker_index = global_marker_index;
}


static void
  createMarker(
    SceneState &state,
    const Point &position,
    bool is_local
  )
{
  MarkerIndex index = createMarkerInState(state, is_local);
  state.marker(index).position = position;
}


static void createLocalMarker(SceneState &state, const Point &position)
{
  createMarker(state,position,/*is_local*/true);
}


static void createGlobalMarker(SceneState &state, const Point &position)
{
  createMarker(state,position,/*is_local*/false);
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

  Transform default_box_transform =
    makeTransform(defaultBoxCoordinateAxes(), defaultBoxTranslation());

  result.box.global = transformState(default_box_transform);

  createLocalMarker(result,  {1,1,0});
  createLocalMarker(result,  {1,1,1});
  createLocalMarker(result,  {0,1,1});
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
