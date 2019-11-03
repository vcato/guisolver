#include "sceneobjects.hpp"

#include "settransform.hpp"
#include "indicesof.hpp"


using TransformHandle = Scene::TransformHandle;
using LineHandle = Scene::LineHandle;

static SceneHandles::Marker
  createSceneLocal(
    Scene &scene,
    TransformHandle &parent,
    Scene::Point position
  )
{
  auto point = scene.createSphere(parent);
  scene.setGeometryScale(point,0.1,0.1,0.1);
  scene.setColor(point,0,0,1);
  scene.setTranslation(point,position);
  SceneHandles::Marker marker_handles = SceneHandles::Marker{point};
  return marker_handles;
}


static SceneHandles::Marker
  createSceneGlobal(
    Scene &scene,
    Scene::Point position
  )
{
  auto point = scene.createSphere();
  scene.setGeometryScale(point, 0.1, 0.1, 0.1);
  scene.setColor(point, 0, 1, 0);
  scene.setTranslation(point,position);
  SceneHandles::Marker marker_handles = SceneHandles::Marker{point};
  return marker_handles;
}


static SceneHandles::Marker
  createSceneMarker(
    Scene &scene,
    const SceneState::Marker &state_marker,
    Scene::TransformHandle box
  )
{
  if (state_marker.is_local) {
    return createSceneLocal(scene,box,state_marker.position);
  }
  else {
    return createSceneGlobal(scene,state_marker.position);
  }
}


static SceneHandles::DistanceError
  createDistanceErrorHandles(Scene &scene)
{
  LineHandle line = scene.createLine(scene.top());
  scene.setColor(line,1,0,0);
  return {line};
}


static SceneHandles::DistanceError
  createSceneDistanceError(
    const SceneState::DistanceError &distance_error_state,
    Scene &scene,
    const SceneHandles::Markers &markers
  )
{
  MarkerIndex start_marker_index = distance_error_state.start_marker_index;
  MarkerIndex end_marker_index = distance_error_state.end_marker_index;

  Scene::TransformHandle start_handle = markers[start_marker_index].handle;
  Scene::TransformHandle end_handle = markers[end_marker_index].handle;
  Scene::Point start = scene.worldPoint({0,0,0},start_handle);
  Scene::Point end = scene.worldPoint({0,0,0},end_handle);

  SceneHandles::DistanceError one_distance_error_handles =
    createDistanceErrorHandles(scene);

  scene.setStartPoint(one_distance_error_handles.line, start);
  scene.setEndPoint(one_distance_error_handles.line, end);

  return one_distance_error_handles;
}


SceneHandles createSceneObjects(const SceneState &state, Scene &scene)
{
  auto box_handle = scene.createBox();

  scene.setGeometryScale(
    box_handle,
    state.box.scale_x,
    state.box.scale_y,
    state.box.scale_z
  );

  setTransform(box_handle, state.box.global, scene);

  vector<SceneHandles::Marker> marker_handles;

  for (auto &state_marker : state.markers) {
    marker_handles.push_back(createSceneMarker(scene,state_marker,box_handle));
  }

  vector<SceneHandles::DistanceError> distance_error_handles;

  for (auto &distance_error_state : state.distance_errors) {
    distance_error_handles.push_back(
      createSceneDistanceError(distance_error_state, scene, marker_handles)
    );
  }

  return {
    box_handle,
    marker_handles,
    distance_error_handles
  };
}


void
  updateBoxInScene(
    Scene &scene,
    const TransformHandle &box_handle,
    const SceneState::Box &box_state
  )
{
  setTransform(box_handle, box_state.global, scene);

  scene.setGeometryScale(
    box_handle, box_state.scale_x, box_state.scale_y, box_state.scale_z
  );
}


static void
  updateDistanceErrorInScene(
    Scene &scene,
    const SceneHandles::DistanceError &distance_error_handles,
    const SceneState::DistanceError &distance_error_state,
    const SceneHandles &scene_handles
  )
{
  TransformHandle line_start =
    scene_handles.markers[
      distance_error_state.start_marker_index
    ].handle;

  TransformHandle line_end =
    scene_handles.markers[
      distance_error_state.end_marker_index
    ].handle;

  Scene::Point start = scene.worldPoint({0,0,0}, line_start);
  Scene::Point end = scene.worldPoint({0,0,0}, line_end);
  scene.setStartPoint(distance_error_handles.line, start);
  scene.setEndPoint(distance_error_handles.line, end);
}


void
  updateDistanceErrorsInScene(
    Scene &scene,
    const SceneHandles &scene_handles,
    const SceneState &scene_state
  )
{
  for (auto i : indicesOf(scene_state.distance_errors)) {
    updateDistanceErrorInScene(
      scene,
      scene_handles.distance_errors[i],
      scene_state.distance_errors[i],
      scene_handles
    );
  }
}
