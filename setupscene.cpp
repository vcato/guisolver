#include "setupscene.hpp"

#include <iostream>
#include "updatescenestatefromscene.hpp"
#include "globaltransform.hpp"

using std::cerr;
using std::ostream;
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


static void
  createDistanceError(
    SceneState &state,
    MarkerIndex local_marker_index,
    MarkerIndex global_marker_index
  )
{
  state.addDistanceError(local_marker_index, global_marker_index);
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


static SceneState defaultState()
{
  SceneState result;

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


static SceneHandles createSceneObjects(const SceneState &state, Scene &scene)
{
  auto box_handle = scene.createBox();
  scene.setGeometryScale(box_handle,5,.1,10);
  scene.setTranslation(box_handle,{0,1,0});

  scene.setCoordinateAxes(
    box_handle,
    CoordinateAxes{
      Scene::Vector(0,0,-1),
      Scene::Vector(0,1, 0),
      Scene::Vector(1,0, 0)
    }
  );

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


SceneData setupScene(Scene &scene)
{
  SceneState state = defaultState();
  SceneHandles handles = createSceneObjects(state, scene);
  state.box_global = globalTransform(scene, handles.box);
  return { handles, state };
}
