#include "makescenestate.hpp"

#include "maketransform.hpp"


static Point
  localTranslation(Scene::TransformHandle transform_id,const Scene &scene)
{
  return scene.translation(transform_id);
}


static void
  addStateMarker(
    SceneState &result,
    const SceneHandles::Marker &setup_marker,
    const Scene &scene
  )
{
  Point position = localTranslation(setup_marker.handle, scene);

  if (setup_marker.is_local) {
    result.addLocalMarker(position);
  }
  else {
    result.addGlobalMarker(position);
  }
}


static void
  addStateDistanceError(
    SceneState &result,
    const SceneHandles::DistanceError &setup_distance_error,
    const Scene &
  )
{
  MarkerIndex start_marker_index = setup_distance_error.start_marker_index;
  MarkerIndex end_marker_index = setup_distance_error.end_marker_index;
  result.addDistanceError(start_marker_index,end_marker_index);
}


static Transform
  globalTransform(const Scene &scene,Scene::TransformHandle transform_id)
{
  Point translation = scene.translation(transform_id);
  CoordinateAxes coordinate_axes = scene.coordinateAxes(transform_id);
  return makeTransform(coordinate_axes,translation);
}


SceneState makeSceneState(const Scene &scene,const SceneHandles &setup)
{
  SceneState result;

  for (const auto &setup_marker : setup.markers) {
    addStateMarker(result, setup_marker, scene);
  }

  for (const auto &setup_line : setup.distance_errors) {
    addStateDistanceError(result, setup_line, scene);
  }

  result.box_global = globalTransform(scene,setup.box);

  return result;
}
