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

using std::cerr;


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


static SceneState::Line
  makeStateLine(const SceneSetup::Line &setup_line, const Scene &scene)
{
  Point start = localTranslation(setup_line.start, scene);
  Point end = localTranslation(setup_line.end, scene);
  return {start,end};
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
    result.lines.push_back(makeStateLine(setup_line,scene));
  }

  result.box_global = globalTransform(scene,setup.box);

  return result;
}


static void changingCallback(Scene &scene,SceneSetup &setup)
{
  // The mouse button is down.  The scene is being changed, but we don't
  // consider this change complete.  We'll update the lines to be drawn
  // between the new positions.  We'll wait to update the box position
  // until the change is complete.

  updateLines(scene,setup);
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


static void changedCallback(Scene &scene,SceneSetup &setup)
{
  SceneState state = sceneState(scene,setup);
  solveBoxPosition(state);
  setTransform(setup.box, state.box_global, scene);
  updateLines(scene,setup);
}


static void fillTree(QtTreeWidget &tree_widget)
{
  using NumericValue = QtTreeWidget::NumericValue;
  using LabelProperties = QtTreeWidget::LabelProperties;
  NumericValue no_minimum = std::numeric_limits<NumericValue>::min();
  NumericValue no_maximum = std::numeric_limits<NumericValue>::max();

  tree_widget.createVoidItem({0},LabelProperties{"[Scene]"});

  tree_widget.createVoidItem(
    {0,0},LabelProperties{"[Transform]"}
  );

  tree_widget.createVoidItem(
    {0,0,0},LabelProperties{"translation: []"}
  );

  tree_widget.createNumericItem(
    {0,0,0,0},LabelProperties{"x:"},0,no_minimum, no_maximum
  );

  tree_widget.createNumericItem(
    {0,0,0,1},LabelProperties{"y:"},0,no_minimum, no_maximum
  );

  tree_widget.createNumericItem(
    {0,0,0,2},LabelProperties{"z:"},0,no_minimum, no_maximum
  );

  tree_widget.createVoidItem(
    {0,0,1},LabelProperties{"rotation: []"}
  );

  tree_widget.createNumericItem(
    {0,0,1,0},LabelProperties{"x:"},0,no_minimum, no_maximum
  );

  tree_widget.createNumericItem(
    {0,0,1,1},LabelProperties{"y:"},0,no_minimum, no_maximum
  );

  tree_widget.createNumericItem(
    {0,0,1,2},LabelProperties{"z:"},0,no_minimum, no_maximum
  );

  tree_widget.createVoidItem(
    {0,0,2},LabelProperties{"[Box]"}
  );

  tree_widget.createVoidItem({0,0,3},LabelProperties{"[Marker]"});
  tree_widget.createVoidItem({0,0,3,0},LabelProperties{"name: \"local1\""});
  tree_widget.createVoidItem({0,0,4},LabelProperties{"[Marker]"});
  tree_widget.createVoidItem({0,0,4,0},LabelProperties{"name: \"local2\""});
  tree_widget.createVoidItem({0,0,5},LabelProperties{"[Marker]"});
  tree_widget.createVoidItem({0,0,5,0},LabelProperties{"name: \"local3\""});
  tree_widget.createVoidItem({0,1},LabelProperties{"[Marker]"});
  tree_widget.createVoidItem({0,1,0},LabelProperties{"name: \"global1\""});
  tree_widget.createVoidItem({0,2},LabelProperties{"[Marker]"});
  tree_widget.createVoidItem({0,2,0},LabelProperties{"name: \"global2\""});
  tree_widget.createVoidItem({0,3},LabelProperties{"[Marker]"});
  tree_widget.createVoidItem({0,3,0},LabelProperties{"name: \"global3\""});
}


int main(int argc,char** argv)
{
  QApplication app(argc,argv);
  QMainWindow main_window;
  main_window.resize(1024,480);
  main_window.show();
  OSGScene scene;

  SceneSetup scene_setup = setupScene(scene);
  scene.changed_callback = [&]{ changedCallback(scene,scene_setup); };
  scene.changing_callback = [&]{ changingCallback(scene,scene_setup); };

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
  fillTree(tree_widget);


  layout.addWidget(graphics_window_ptr->getGLWidget());
  app.exec();
}
