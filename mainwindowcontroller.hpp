#include "scene.hpp"
#include "treewidget.hpp"
#include "treepaths.hpp"
#include "scenestate.hpp"
#include "scenehandles.hpp"
#include "scenestate.hpp"


struct MainWindowData {
  Scene &scene;
  TreeWidget &tree_widget;
  SceneState scene_state;
  SceneHandles scene_handles;
  TreePaths tree_paths;

  MainWindowData(Scene &,TreeWidget &);
};


struct MainWindowController {
  MainWindowData main_window_data;
  MainWindowController(Scene &scene,TreeWidget &tree_widget);
};
