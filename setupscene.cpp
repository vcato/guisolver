#include "setupscene.hpp"

#include <iostream>
#include "updatescenestatefromscene.hpp"

using std::cerr;
using std::ostream;
using TransformHandle = Scene::TransformHandle;
using LineHandle = Scene::LineHandle;


namespace {
struct MarkerData {
  SceneHandles::Marker handles;
  SceneDescription::Marker description;
};
}


static SceneHandles::Marker
  createLocal(
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

static MarkerData
  createLocalData(
    Scene &scene,
    TransformHandle &parent,
    Scene::Point position
  )
{
  SceneDescription::Marker marker_description = {/*is_local*/true};
  SceneHandles::Marker marker_handles = createLocal(scene,parent,position);
  return {marker_handles,marker_description};
}


static SceneHandles::Marker
  createGlobal(
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


static MarkerData createGlobalData(Scene &scene,Scene::Point position)
{
  SceneDescription::Marker marker_description = {/*is_local*/false};
  SceneHandles::Marker marker_handles = createGlobal(scene,position);
  return {marker_handles,marker_description};
}


static void
  createLine(
    vector<SceneHandles::DistanceError> &distance_error_handles,
    vector<SceneDescription::DistanceError> &distance_error_descriptions,
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
  LineHandle line = scene.createLine(scene.top());
  scene.setColor(line,1,0,0);
  scene.setStartPoint(line,start);
  scene.setEndPoint(line,end);

  distance_error_handles.push_back({line});

  distance_error_descriptions.push_back({
    local_marker_index,
    global_marker_index
  });
}


SceneData setupScene(Scene &scene)
{
  auto box = scene.createBox();
  scene.setGeometryScale(box,5,.1,10);
  scene.setTranslation(box,{0,1,0});

  scene.setCoordinateAxes(
    box,
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
  MarkerData marker1 = createLocalData(scene,box,local1);
  MarkerData marker2 = createLocalData(scene,box,local2);
  MarkerData marker3 = createLocalData(scene,box,local3);
  MarkerData marker4 = createGlobalData(scene,global1);
  MarkerData marker5 = createGlobalData(scene,global2);
  MarkerData marker6 = createGlobalData(scene,global3);
  marker_handles.push_back(marker1.handles);
  marker_handles.push_back(marker2.handles);
  marker_handles.push_back(marker3.handles);
  marker_handles.push_back(marker4.handles);
  marker_handles.push_back(marker5.handles);
  marker_handles.push_back(marker6.handles);
  vector<SceneDescription::Marker> marker_descriptions;
  marker_descriptions.push_back(marker1.description);
  marker_descriptions.push_back(marker2.description);
  marker_descriptions.push_back(marker3.description);
  marker_descriptions.push_back(marker4.description);
  marker_descriptions.push_back(marker5.description);
  marker_descriptions.push_back(marker6.description);

  vector<SceneHandles::DistanceError> distance_error_handles;
  vector<SceneDescription::DistanceError> distance_error_descriptions;
  int n_lines = 3;

  for (int i=0; i!=n_lines; ++i) {
    MarkerIndex local_marker_index = i;
    MarkerIndex global_marker_index = i + 3;
    createLine(
      distance_error_handles,
      distance_error_descriptions,
      marker_handles,
      scene,
      local_marker_index,
      global_marker_index
    );
  }

  SceneHandles scene_handles = { box, marker_handles, distance_error_handles };

  SceneDescription scene_description = {
    marker_descriptions,
    distance_error_descriptions
  };

  SceneState state;
  updateSceneStateFromScene(state, scene, scene_handles, scene_description);

  return {
    scene_handles,
    scene_description,
    state
  };
}
