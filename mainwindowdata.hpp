#ifndef MAINWINDOWDATA_HPP_
#define MAINWINDOWDATA_HPP_

#include "scene.hpp"
#include "treewidget.hpp"
#include "scenesetup.hpp"
#include "treepaths.hpp"


struct MainWindowData {
  Scene &scene;
  TreeWidget &tree_widget;
  SceneSetup scene_setup;
  TreePaths tree_paths;

  MainWindowData(Scene &scene_arg,TreeWidget &tree_widget_arg);
};


#endif /* MAINWINDOWDATA_HPP_ */
