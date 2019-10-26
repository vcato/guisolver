#ifndef SCENEHANDLES_HPP_
#define SCENEHANDLES_HPP_

#include "vector.hpp"
#include "scene.hpp"
#include "markerindex.hpp"

#define REMOVE_IS_LOCAL_FROM_SCENE_HANDLES 0


struct SceneHandles {
  struct Marker;
  struct DistanceError;
  using TransformHandles = vector<Scene::TransformHandle>;
  using Markers = vector<Marker>;
  using DistanceErrors = vector<DistanceError>;

  struct Marker {
    Scene::TransformHandle handle;
#if !REMOVE_IS_LOCAL_FROM_SCENE_HANDLES
    bool is_local;
#endif
  };

  struct DistanceError {
    Scene::LineHandle line_handle;
    MarkerIndex start_marker_index;
    MarkerIndex end_marker_index;
  };

  Scene::TransformHandle box;
  Markers markers;
  DistanceErrors distance_errors;
};


#endif /* SCENEHANDLES_HPP_ */

