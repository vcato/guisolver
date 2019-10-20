#include "mainwindowdata.hpp"

#include "setupscene.hpp"
#include "filltree.hpp"


MainWindowData::MainWindowData(Scene &scene_arg,TreeWidget &tree_widget_arg)
: scene(scene_arg),
  tree_widget(tree_widget_arg),
  scene_setup(setupScene(scene)),
  tree_paths(fillTree(tree_widget))
{
}
