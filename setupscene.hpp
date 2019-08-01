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
