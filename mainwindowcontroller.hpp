#include "scene.hpp"
#include "treewidget.hpp"
#include "treepaths.hpp"
#include "scenestate.hpp"
#include "scenedata.hpp"


struct MainWindowData {
  Scene &scene;
  TreeWidget &tree_widget;
  SceneData scene_data;
  SceneState scene_state;
  TreePaths tree_paths;

  MainWindowData(Scene &,TreeWidget &);
};


struct MainWindowController {
  MainWindowData main_window_data;
  MainWindowController(Scene &scene,TreeWidget &tree_widget);
};
