#ifndef SCENEHANDLES_HPP_
#define SCENEHANDLES_HPP_

#include "vector.hpp"
#include "scene.hpp"
#include "markerindex.hpp"

#define ADD_SCENE_DESCRIPTION 0
#define USE_SCENE_DATA 1


#if ADD_SCENE_DESCRIPTION
struct SceneDescription {
  struct Marker {
    bool is_local;
  };

  struct DistanceError {
    MarkerIndex start_marker_index;
    MarkerIndex end_marker_index;
  };

  using Markers = vector<Marker>;
  using DistanceErrors = vector<DistanceError>;

  Markers markers;
  DistanceErrors distance_errors;
};
#endif


struct SceneHandles {
  struct Marker;
  struct DistanceError;
  using TransformHandles = vector<Scene::TransformHandle>;
  using Markers = vector<Marker>;
  using DistanceErrors = vector<DistanceError>;

  struct Marker {
    Scene::TransformHandle handle;
#if !ADD_SCENE_DESCRIPTION
    bool is_local;
#endif
  };

  struct DistanceError {
    Scene::LineHandle line_handle;
#if !ADD_SCENE_DESCRIPTION
    MarkerIndex start_marker_index;
    MarkerIndex end_marker_index;
#endif
  };

  Scene::TransformHandle box;
  Markers markers;
  DistanceErrors distance_errors;
};


#if USE_SCENE_DATA
struct SceneData {
  SceneHandles handles;
#if ADD_SCENE_DESCRIPTION
  SceneDescription description;
#endif
};
#endif


#endif /* SCENEHANDLES_HPP_ */

