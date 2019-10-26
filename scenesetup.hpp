#ifndef SCENESETUP_HPP_
#define SCENESETUP_HPP_

#include "vector.hpp"
#include "scene.hpp"
#include "markerindex.hpp"


struct SceneSetup {
  struct Marker;
  using TransformHandles = vector<Scene::TransformHandle>;
  using Markers = vector<Marker>;

  struct Marker {
    Scene::TransformHandle handle;
    bool is_local;
  };

  struct Line {
    Scene::LineHandle handle;
    MarkerIndex start_marker_index;
    MarkerIndex end_marker_index;
  };

  Scene::TransformHandle box;
  Markers markers;
  vector<Line> lines;
};


#endif /* SCENESETUP_HPP_ */
