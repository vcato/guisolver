#include "createsceneobjects.hpp"

#include "settransform.hpp"


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
  scene.setGeometryScale(box_handle,5,.1,10);
  setTransform(box_handle, state.box_global, scene);

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
