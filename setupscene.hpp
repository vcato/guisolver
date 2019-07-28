#include "scene.hpp"

struct SceneSetup {
  struct Line {
    Scene::LineHandle handle;
    Scene::TransformHandle start;
    Scene::TransformHandle end;
  };

  std::vector<Line> lines;
};


extern SceneSetup setupScene(Scene &scene);
