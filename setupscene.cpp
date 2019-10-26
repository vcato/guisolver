#include "setupscene.hpp"

#include <iostream>

using std::cerr;
using std::ostream;
using TransformHandle = Scene::TransformHandle;
using LineHandle = Scene::LineHandle;


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
#if !ADD_SCENE_DESCRIPTION
  return SceneHandles::Marker{point,/*is_local*/true};
#else
  return SceneHandles::Marker{point};
#endif
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
#if !ADD_SCENE_DESCRIPTION
  return SceneHandles::Marker{point,/*is_local*/false};
#else
  return SceneHandles::Marker{point};
#endif
}


static void
  createLine(
    vector<SceneHandles::DistanceError> &distance_error_handles,
#if ADD_SCENE_DESCRIPTION
    vector<SceneDescription::DistanceError> &distance_error_descriptions,
#endif
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

#if !ADD_SCENE_DESCRIPTION
  distance_error_handles.push_back({
    line,
    local_marker_index,
    global_marker_index
  });
#else
  distance_error_handles.push_back({line});

  distance_error_descriptions.push_back({
    local_marker_index,
    global_marker_index
  });
#endif
}


SceneHandles setupScene(Scene &scene)
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

  vector<SceneHandles::Marker> markers;
  markers.push_back(createLocal(scene,box,local1));
  markers.push_back(createLocal(scene,box,local2));
  markers.push_back(createLocal(scene,box,local3));
  markers.push_back(createGlobal(scene,global1));
  markers.push_back(createGlobal(scene,global2));
  markers.push_back(createGlobal(scene,global3));

  vector<SceneHandles::DistanceError> distance_error_handles;
#if ADD_SCENE_DESCRIPTION
  vector<SceneDescription::DistanceError> distance_error_descriptions;
#endif
  int n_lines = 3;

  for (int i=0; i!=n_lines; ++i) {
    MarkerIndex local_marker_index = i;
    MarkerIndex global_marker_index = i + 3;
#if !ADD_SCENE_DESCRIPTION
    createLine(
      distance_error_handles,
      markers,
      scene,
      local_marker_index,
      global_marker_index
    );
#else
    createLine(
      distance_error_handles,
      distance_error_descriptions,
      markers,
      scene,
      local_marker_index,
      global_marker_index
    );
#endif
  }

  return { box, markers, distance_error_handles };
}
