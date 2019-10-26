#include "scene.hpp"
#include "treewidget.hpp"
#include "scenehandles.hpp"
#include "treepaths.hpp"


struct MainWindowData {
  Scene &scene;
  TreeWidget &tree_widget;
  SceneHandles scene_setup;
  TreePaths tree_paths;

  MainWindowData(Scene &,TreeWidget &);
};


struct MainWindowController {
  MainWindowData main_window_data;
  MainWindowController(Scene &scene,TreeWidget &tree_widget);
};
