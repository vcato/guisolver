#ifndef SCENESETUP_HPP_
#define SCENESETUP_HPP_

#include <vector>
#include "scene.hpp"


struct SceneSetup {
  using TransformHandles = std::vector<Scene::TransformHandle>;

  struct Line {
    Scene::LineHandle handle;
    Scene::TransformHandle start;
    Scene::TransformHandle end;
  };

  Scene::TransformHandle box;
  TransformHandles locals;
  TransformHandles globals;
  std::vector<Line> lines;
};


#endif /* SCENESETUP_HPP_ */
