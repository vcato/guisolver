#ifndef SETUPSCENE_HPP_
#define SETUPSCENE_HPP_

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


extern SceneSetup setupScene(Scene &scene);

#endif /* SETUPSCENE_HPP_ */
