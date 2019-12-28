#include "scenestate.hpp"
#include "scene.hpp"
#include "scenehandles.hpp"
#include "treewidget.hpp"
#include "treepaths.hpp"


class MainWindowController {
  public:
    MainWindowController(SceneState &,Scene &,TreeWidget &);

    void replaceSceneStateWith(const SceneState &);

  private:
    struct Impl;

    struct Data {
      SceneState &scene_state;
      Scene &scene;
      TreeWidget &tree_widget;

      SceneHandles scene_handles;
      TreePaths tree_paths;

      Data(SceneState &scene_state, Scene &, TreeWidget &);
    };

    Data data;
};
