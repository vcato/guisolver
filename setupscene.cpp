#include "setupscene.hpp"

#include <iostream>
#include "updatescenestatefromscene.hpp"
#include "globaltransform.hpp"
#include "settransform.hpp"
#include "maketransform.hpp"
#include "createsceneobjects.hpp"

using std::cerr;
using std::ostream;


static void
  createDistanceError(
    SceneState &state,
    MarkerIndex local_marker_index,
    MarkerIndex global_marker_index
  )
{
  state.addDistanceError(local_marker_index, global_marker_index);
}


static void
  createMarker(
    SceneState &state,
    const Point &position,
    bool is_local,
    const SceneState::String &name
  )
{
  state.markers.push_back(SceneState::Marker{position, is_local, name});
}


static void
  createLocalMarker(
    SceneState &state,
    const Point &position,
    const SceneState::String &name
  )
{
  createMarker(state,position,/*is_local*/true,name);
}


static void
  createGlobalMarker(
    SceneState &state,
    const Point &position,
    const SceneState::String &name
  )
{
  createMarker(state,position,/*is_local*/false,name);
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

  result.box.global =
    makeTransform(defaultBoxCoordinateAxes(),defaultBoxTranslation());

  createLocalMarker(result,  {1,1,0}, "local1");
  createLocalMarker(result,  {1,1,1}, "local2");
  createLocalMarker(result,  {0,1,1}, "local3");
  createGlobalMarker(result, {1,0,0}, "global1");
  createGlobalMarker(result, {1,0,1}, "global2");
  createGlobalMarker(result, {0,0,1}, "global3");

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
