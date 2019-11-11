#include "sceneobjects.hpp"

#include <iostream>
#include "settransform.hpp"
#include "maketransform.hpp"
#include "indicesof.hpp"
#include "globaltransform.hpp"
#include "removeindexfrom.hpp"

using std::cerr;
using TransformHandle = Scene::TransformHandle;
using LineHandle = Scene::LineHandle;

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
  updateStateMarkerPositions(
    SceneState &scene_state,
    const SceneHandles::Markers &handles_markers,
    const Scene &scene
  )
{
  for (auto i : indicesOf(handles_markers)) {
    updateMarkerPosition(scene_state.marker(i), handles_markers[i], scene);
  }
}


void
  updateSceneStateFromSceneObjects(
    SceneState &state,
    const Scene &scene,
    const SceneHandles &scene_handles
  )
{
  updateStateMarkerPositions(state, scene_handles.markers, scene);
  state.box.global = globalTransform(scene, scene_handles.box);
}


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
  createSceneMarker1(
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


static SceneHandles::DistanceError createDistanceErrorInScene1(Scene &scene)
{
  LineHandle line = scene.createLine(scene.top());
  scene.setColor(line,1,0,0);
  return {line};
}



void
  createDistanceErrorInScene(
    Scene &scene,
    SceneHandles &scene_handles,
    const SceneState::DistanceError &
  )
{
  vector<SceneHandles::DistanceError> &distance_error_handles =
    scene_handles.distance_errors;

  distance_error_handles.push_back(createDistanceErrorInScene1(scene));
}


void
  removeDistanceErrorFromScene(
    Scene &scene,
    SceneHandles::DistanceErrors &distance_errors,
    int index
  )
{
  scene.destroyLine(distance_errors[index].line);
  removeIndexFrom(distance_errors, index);
}


void
  removeMarkerFromScene(
    Scene &scene,
    SceneHandles::Markers &markers,
    MarkerIndex index
  )
{
  scene.destroyObject(markers[index].handle);
  removeIndexFrom(markers, index);
}


void
  createMarkerInScene(
    Scene &scene,
    SceneHandles &scene_handles,
    const SceneState &state,
    MarkerIndex marker_index
  )
{
  const SceneState::Marker &state_marker = state.marker(marker_index);
  TransformHandle box_handle = scene_handles.box;
  vector<SceneHandles::Marker> &marker_handles = scene_handles.markers;
  marker_handles.push_back(createSceneMarker1(scene,state_marker,box_handle));
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

  SceneHandles scene_handles;
  scene_handles.box = box_handle;

  setTransform(box_handle, state.box.global, scene);

  for (MarkerIndex marker_index : indicesOf(state.markers())) {
    createMarkerInScene(scene, scene_handles, state, marker_index);
  }

  for (auto &distance_error : state.distance_errors) {
    createDistanceErrorInScene(scene, scene_handles, distance_error);
  }

  return scene_handles;
}


static void
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
  bool have_both_markers =
    distance_error_state.optional_start_marker_index &&
    distance_error_state.optional_end_marker_index;

  Scene::Point start = {0,0,0};
  Scene::Point end = {0,0,0};

  if (have_both_markers) {
    MarkerIndex start_marker_index =
      *distance_error_state.optional_start_marker_index;

    MarkerIndex end_marker_index =
      *distance_error_state.optional_end_marker_index;

    TransformHandle line_start =
      scene_handles.markers[start_marker_index].handle;

    TransformHandle line_end =
      scene_handles.markers[end_marker_index].handle;

    start = scene.worldPoint({0,0,0}, line_start);
    end = scene.worldPoint({0,0,0}, line_end);
  }

  scene.setStartPoint(distance_error_handles.line, start);
  scene.setEndPoint(distance_error_handles.line, end);
}


static void
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


static void
  updateMarkerInScene(
    Scene &scene,
    const SceneHandles::Marker &marker_handles,
    const SceneState::Marker &marker_state
  )
{
  scene.setTranslation(marker_handles.handle, marker_state.position);
}


static void
  updateMarkersInScene(
    Scene &scene,
    const SceneHandles::Markers &markers_handles,
    const SceneState::Markers &marker_states
  )
{
  for (auto i : indicesOf(marker_states)) {
    updateMarkerInScene(scene, markers_handles[i], marker_states[i]);
  }
}


void
  updateSceneObjects(
    Scene &scene,
    const SceneHandles &scene_handles,
    const SceneState &state
  )
{
  updateBoxInScene(scene, scene_handles.box, state.box);
  updateMarkersInScene(scene, scene_handles.markers, state.markers());
  updateDistanceErrorsInScene(scene, scene_handles, state);
}
