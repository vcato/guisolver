#ifndef SCENEHANDLES_HPP_
#define SCENEHANDLES_HPP_

#include "vector.hpp"
#include "scene.hpp"
#include "markerindex.hpp"


struct SceneHandles {
  struct Marker;
  struct DistanceError;
  using TransformHandles = vector<Scene::TransformHandle>;
  using Markers = vector<Marker>;
  using DistanceErrors = vector<DistanceError>;

  struct Marker {
    Scene::TransformHandle handle;
    bool is_local;
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

