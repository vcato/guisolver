#ifndef SCENESETUP_HPP_
#define SCENESETUP_HPP_

#include <vector>
#include "scene.hpp"
#include "markerindex.hpp"


struct SceneSetup {
  struct Marker;
  using TransformHandles = std::vector<Scene::TransformHandle>;
  using Markers = std::vector<Marker>;

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
  std::vector<Line> lines;
};


#endif /* SCENESETUP_HPP_ */
