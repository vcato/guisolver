#include "updatescenestatefromscene.hpp"

#include "maketransform.hpp"
#include "indicesof.hpp"


static Point
  localTranslation(Scene::TransformHandle transform_id,const Scene &scene)
{
  return scene.translation(transform_id);
}


static void
  updateMarkerPosition(
    SceneState::Marker &state_marker,
    const SceneHandles::Marker &handles_marker,
    const Scene &scene
  )
{
  state_marker.position = localTranslation(handles_marker.handle, scene);
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


static void
  updateStateMarkerPositions(
    SceneState::Markers &state_markers,
    const SceneHandles::Markers &handles_markers,
    const Scene &scene
  )
{
  for (auto i : indicesOf(handles_markers)) {
    const SceneHandles::Marker &handles_marker = handles_markers[i];
    updateMarkerPosition(state_markers[i], handles_marker, scene);
  }
}


void
  updateSceneStateFromScene(
    SceneState &state,
    const Scene &scene,
    const SceneHandles &scene_handles
  )
{
  state = SceneState();

  for (auto i : indicesOf(scene_handles.markers)) {
    const SceneHandles::Marker &handles_marker = scene_handles.markers[i];
    MarkerIndex marker_index = state.addMarker();
    state.markers[marker_index].is_local = handles_marker.is_local;
  }

  for (const auto &setup_line : scene_handles.distance_errors) {
    addStateDistanceError(state, setup_line, scene);
  }

  updateStateMarkerPositions(state.markers, scene_handles.markers, scene);

  state.box_global = globalTransform(scene, scene_handles.box);
}
