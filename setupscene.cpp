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


static void
  createDistanceError(
    vector<SceneState::DistanceError> &distance_error_states,
    vector<SceneHandles::DistanceError> &distance_error_handles,
    const SceneHandles::Markers &markers,
    Scene &scene,
    MarkerIndex local_marker_index,
    MarkerIndex global_marker_index
  )
{
  Scene::TransformHandle local_handle = markers[local_marker_index].handle;
  Scene::TransformHandle global_handle = markers[global_marker_index].handle;
  Scene::Point start = scene.worldPoint({0,0,0},local_handle);
  Scene::Point end = scene.worldPoint({0,0,0},global_handle);

  SceneHandles::DistanceError one_distance_error_handles =
    createDistanceErrorHandles(scene);

  scene.setStartPoint(one_distance_error_handles.line, start);
  scene.setEndPoint(one_distance_error_handles.line, end);
  distance_error_handles.push_back(one_distance_error_handles);

  distance_error_states.push_back({
    local_marker_index,
    global_marker_index
  });
}


static void
  createLocalMarker(
    SceneState::Markers &marker_states,
    vector<SceneHandles::Marker> &marker_handles,
    Scene &scene,
    Scene::TransformHandle box,
    const Point &position
  )
{
  marker_states.push_back(SceneState::Marker{position,/*is_local*/true});

  SceneHandles::Marker one_marker_handles =
    createSceneLocal(scene,box,position);

  marker_handles.push_back(one_marker_handles);
}


static void
  createGlobalMarker(
    SceneState::Markers &marker_states,
    vector<SceneHandles::Marker> &marker_handles,
    Scene &scene,
    const Point &position
  )
{
  marker_states.push_back(SceneState::Marker{position,/*is_local*/false});
  SceneHandles::Marker one_marker_handles = createSceneGlobal(scene,position);
  marker_handles.push_back(one_marker_handles);
}


SceneData setupScene(Scene &scene)
{
  auto box_handle = scene.createBox();
  scene.setGeometryScale(box_handle,5,.1,10);
  scene.setTranslation(box_handle,{0,1,0});

  scene.setCoordinateAxes(
    box_handle,
    CoordinateAxes{
      Scene::Vector(0,0,-1),
      Scene::Vector(0,1,0),
      Scene::Vector(1,0,0)
    }
  );

  Scene::Point local1 = {1,1,0};
  Scene::Point local2 = {1,1,1};
  Scene::Point local3 = {0,1,1};
  Scene::Point global1 = {1,0,0};
  Scene::Point global2 = {1,0,1};
  Scene::Point global3 = {0,0,1};

  vector<SceneHandles::Marker> marker_handles;
  vector<SceneState::Marker> marker_states;

  createLocalMarker(marker_states, marker_handles, scene, box_handle, local1);
  createLocalMarker(marker_states, marker_handles, scene, box_handle, local2);
  createLocalMarker(marker_states, marker_handles, scene, box_handle, local3);
  createGlobalMarker(marker_states, marker_handles, scene, global1);
  createGlobalMarker(marker_states, marker_handles, scene, global2);
  createGlobalMarker(marker_states, marker_handles, scene, global3);

  vector<SceneHandles::DistanceError> distance_error_handles;
  vector<SceneState::DistanceError> distance_error_states;
  int n_lines = 3;

  for (int i=0; i!=n_lines; ++i) {
    MarkerIndex local_marker_index = i;
    MarkerIndex global_marker_index = i + 3;

    createDistanceError(
      distance_error_states,
      distance_error_handles,
      marker_handles,
      scene,
      local_marker_index,
      global_marker_index
    );
  }

  SceneHandles scene_handles = { box_handle, marker_handles, distance_error_handles };


  Transform box_global = globalTransform(scene, box_handle);
  SceneState state = { marker_states, distance_error_states, box_global };

  return {
    scene_handles,
    state
  };
}
