#ifndef SCENESETUP_HPP_
#define SCENESETUP_HPP_

#include "scene.hpp"


struct SceneSetup {
  struct Line {
    Scene::LineHandle handle;
    Scene::TransformHandle start;
    Scene::TransformHandle end;
  };

  Scene::TransformHandle box;
  std::vector<Line> lines;
};


#endif /* SCENESETUP_HPP_ */
