#include "mainwindowcontroller.hpp"

#include <iostream>
#include "streamvector.hpp"
#include "transform.hpp"
#include "eigenconv.hpp"
#include "maketransform.hpp"
#include "scenestate.hpp"
#include "updatetreevalues.hpp"
#include "sceneerror.hpp"
#include "scenesolver.hpp"
#include "setupscene.hpp"
#include "filltree.hpp"

using std::cerr;


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


static Transform
  globalTransform(const Scene &scene,Scene::TransformHandle transform_id)
{
  Point translation = scene.translation(transform_id);
  CoordinateAxes coordinate_axes = scene.coordinateAxes(transform_id);
  return makeTransform(coordinate_axes,translation);
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


static SceneState makeSceneState(const Scene &scene,const SceneSetup &setup)
{
  SceneState result;

  for (const auto &setup_line : setup.lines) {
    addStateLine(result, setup_line, scene);
  }

  result.box_global = globalTransform(scene,setup.box);

  return result;
}


static void updateLines(Scene &scene,SceneSetup &setup)
{
  for (auto &line : setup.lines) {
    Scene::Point start = scene.worldPoint({0,0,0}, line.start);
    Scene::Point end = scene.worldPoint({0,0,0}, line.end);
    scene.setStartPoint(line.handle,start);
    scene.setEndPoint(line.handle,end);
  }
}


static void sceneChangingCallback(MainWindowData &main_window_data)
{
  // The mouse button is down.  The scene is being changed, but we don't
  // consider this change complete.  We'll update the lines to be drawn
  // between the new positions.  We'll wait to update the box position
  // until the change is complete.

  Scene &scene = main_window_data.scene;
  SceneSetup &setup = main_window_data.scene_setup;
  updateLines(scene,setup);
  TreeWidget &tree_widget = main_window_data.tree_widget;
  TreePaths &tree_paths = main_window_data.tree_paths;
  updateTreeValues(tree_widget, tree_paths, makeSceneState(scene,setup));
  cerr << sceneError(makeSceneState(scene,setup)) << "\n";
}


static void sceneChangedCallback(MainWindowData &main_window_data)
{
  SceneSetup &setup = main_window_data.scene_setup;
  Scene &scene = main_window_data.scene;
  SceneState state = makeSceneState(scene,setup);
  solveBoxPosition(state);
  setTransform(setup.box, state.box_global, scene);

  TreePaths &tree_paths = main_window_data.tree_paths;
  TreeWidget &tree_widget = main_window_data.tree_widget;
  updateTreeValues(tree_widget, tree_paths, state);

  updateLines(scene,setup);
}


MainWindowData::MainWindowData(
  Scene &scene_arg,
  TreeWidget &tree_widget_arg
)
: scene(scene_arg),
  tree_widget(tree_widget_arg),
  scene_setup(setupScene(scene)),
  tree_paths(fillTree(tree_widget))
{
}


MainWindowController::MainWindowController(Scene &scene,TreeWidget &tree_widget)
: main_window_data{scene,tree_widget}
{
  TreePaths &tree_paths = main_window_data.tree_paths;
  SceneSetup &scene_setup = main_window_data.scene_setup;

  updateTreeValues(
    tree_widget,
    tree_paths,
    makeSceneState(scene, scene_setup)
  );

  scene.changed_callback = [&]{ sceneChangedCallback(main_window_data); };
  scene.changing_callback = [&]{ sceneChangingCallback(main_window_data); };

  tree_widget.spin_box_item_value_changed_function =
    [](const TreePath &path, int value) {
      cerr << "Handling spin_box_item_value_changed_function\n";
      cerr << "  path: " << path << "\n";
      cerr << "  value: " << value << "\n";
    };
}
