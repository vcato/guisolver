#ifndef SCENEHANDLES_HPP_
#define SCENEHANDLES_HPP_

#include "vector.hpp"
#include "scene.hpp"
#include "markerindex.hpp"


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


struct SceneHandles {
  struct Marker;
  struct DistanceError;
  using TransformHandles = vector<Scene::TransformHandle>;
  using Markers = vector<Marker>;
  using DistanceErrors = vector<DistanceError>;

  struct Marker {
    Scene::TransformHandle handle;
  };

  struct DistanceError {
    Scene::LineHandle line_handle;
  };

  Scene::TransformHandle box;
  Markers markers;
  DistanceErrors distance_errors;
};


#endif /* SCENEHANDLES_HPP_ */

