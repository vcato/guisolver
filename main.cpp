#include <iostream>
#include <QApplication>
#include <QMainWindow>
#include <QBoxLayout>
#include "osgscene.hpp"
#include "setupscene.hpp"
#include "sceneerror.hpp"
#include "maketransform.hpp"
#include "scenesolver.hpp"
#include "eigenconv.hpp"
#include "qtwidget.hpp"
#include "qttreewidget.hpp"
#include "qtlayout.hpp"
#include "filltree.hpp"
#include "streamvector.hpp"
#include "updatetreevalues.hpp"

using std::cerr;
using std::string;


static void updateLines(Scene &scene,SceneSetup &setup)
{
  for (auto &line : setup.lines) {
    Scene::Point start = scene.worldPoint({0,0,0}, line.start);
    Scene::Point end = scene.worldPoint({0,0,0}, line.end);
    scene.setStartPoint(line.handle,start);
    scene.setEndPoint(line.handle,end);
  }
}


static Point
  localTranslation(Scene::TransformHandle transform_id,const Scene &scene)
{
  return scene.translation(transform_id);
}


static void
  addStateLine(
    SceneState &result,
    const SceneSetup::Line &setup_line,
    const Scene &scene
  )
{
  Point start = localTranslation(setup_line.start, scene);
  Point end = localTranslation(setup_line.end, scene);
  result.addLine(start,end);
}


static Transform
  globalTransform(const Scene &scene,Scene::TransformHandle transform_id)
{
  Point translation = scene.translation(transform_id);
  CoordinateAxes coordinate_axes = scene.coordinateAxes(transform_id);
  return makeTransform(coordinate_axes,translation);
}


static SceneState sceneState(const Scene &scene,const SceneSetup &setup)
{
  SceneState result;

  for (const auto &setup_line : setup.lines) {
    addStateLine(result, setup_line, scene);
  }

  result.box_global = globalTransform(scene,setup.box);

  return result;
}


namespace {
struct MainWindowData {
  OSGScene &scene;
  QtTreeWidget &tree_widget;
  SceneSetup scene_setup;
  TreePaths tree_paths;

  MainWindowData(OSGScene &scene_arg,QtTreeWidget &tree_widget_arg)
  : scene(scene_arg),
    tree_widget(tree_widget_arg),
    scene_setup(setupScene(scene)),
    tree_paths(fillTree(tree_widget))
  {
  }
};
}


static void changingCallback(MainWindowData &main_window_data)
{
  // The mouse button is down.  The scene is being changed, but we don't
  // consider this change complete.  We'll update the lines to be drawn
  // between the new positions.  We'll wait to update the box position
  // until the change is complete.

  Scene &scene = main_window_data.scene;
  SceneSetup &setup = main_window_data.scene_setup;
  updateLines(scene,setup);
  QtTreeWidget &tree_widget = main_window_data.tree_widget;
  TreePaths &tree_paths = main_window_data.tree_paths;
  updateTreeValues(tree_widget, tree_paths, sceneState(scene,setup));
  cerr << sceneError(sceneState(scene,setup)) << "\n";
}


static void
  setTransform(
    Scene::TransformHandle transform_id,
    const Transform &transform_value,
    Scene &scene
  )
{
  CoordinateAxes coordinate_axes =
    coordinateAxes(transform_value.rotation());

  scene.setCoordinateAxes(transform_id, coordinate_axes);
  scene.setTranslation(transform_id, transform_value.translation());
}


static void changedCallback(MainWindowData &main_window_data)
{
  SceneSetup &setup = main_window_data.scene_setup;
  Scene &scene = main_window_data.scene;
  SceneState state = sceneState(scene,setup);
  solveBoxPosition(state);
  setTransform(setup.box, state.box_global, scene);

  TreePaths &tree_paths = main_window_data.tree_paths;
  QtTreeWidget &tree_widget = main_window_data.tree_widget;
  updateTreeValues(tree_widget, tree_paths, state);

  updateLines(scene,setup);
}


int main(int argc,char** argv)
{
  QApplication app(argc,argv);
  QMainWindow main_window;
  main_window.resize(1024,480);
  main_window.show();
  OSGScene scene;

  GraphicsWindowPtr graphics_window_ptr =
    scene.createGraphicsWindow(ViewType::free);

  // The default size here doesn't seem to really work.  It defaults to
  // a size that is much smaller than 640x480, but if we don't set the default
  // size at all, then it ends up collapsing to 0 size.
  graphics_window_ptr->getGLWidget()->setDefaultSize(QSize(640,480));

  QWidget central_widget;
  main_window.setCentralWidget(&central_widget);
  QBoxLayout &layout = createLayout<QHBoxLayout>(central_widget);

  QtTreeWidget &tree_widget = createWidget<QtTreeWidget>(layout);

  tree_widget.spin_box_item_value_changed_function =
    [](const TreePath &path, int value) {
      cerr << "Handling spin_box_item_value_changed_function\n";
      cerr << "  path: " << path << "\n";
      cerr << "  value: " << value << "\n";
    };

  layout.addWidget(graphics_window_ptr->getGLWidget());

  MainWindowData main_window_data{scene,tree_widget};

  scene.changed_callback = [&]{ changedCallback(main_window_data); };
  scene.changing_callback = [&]{ changingCallback(main_window_data); };

  app.exec();
}
