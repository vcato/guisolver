#include "scenestate.hpp"
#include "scene.hpp"
#include "scenehandles.hpp"
#include "treewidget.hpp"
#include "treepaths.hpp"


class MainWindowController {
  public:
    MainWindowController(Scene &scene,TreeWidget &tree_widget);

  private:
    struct Impl;

    struct Data {
      Scene &scene;
      TreeWidget &tree_widget;
      SceneState scene_state;
      SceneHandles scene_handles;
      TreePaths tree_paths;

      Data(Scene &,TreeWidget &);
    };

    Data data;
};
