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
    const SceneDescription::DistanceError &distance_error_description,
    const Scene &
  )
{
  MarkerIndex start_marker_index = distance_error_description.start_marker_index;
  MarkerIndex end_marker_index = distance_error_description.end_marker_index;
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
    const SceneHandles &scene_handles,
    const SceneDescription &scene_description
  )
{
  state = SceneState();

  for (auto i : indicesOf(scene_handles.markers)) {
    const SceneDescription::Marker &marker_description =
      scene_description.markers[i];
    MarkerIndex marker_index = state.addMarker();
    state.markers[marker_index].is_local = marker_description.is_local;
  }

  for (
    const auto &distance_error_description : scene_description.distance_errors
  ) {
    addStateDistanceError(state, distance_error_description, scene);
  }

  updateStateMarkerPositions(state.markers, scene_handles.markers, scene);

  state.box_global = globalTransform(scene, scene_handles.box);
}
