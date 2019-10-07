#include <iostream>
#include <QApplication>
#include <QMainWindow>
#include "osgscene.hpp"
#include "setupscene.hpp"
#include "sceneerror.hpp"

#define USE_SOLVER 0

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


#if 0
static float constraintError(const Constraint &constraint)
{
  float distance = distanceBetween(predicted,global);
  float error = distance - desired_distance;
  return error*error;
}
#endif


#if 0
static float error()
{
  float error = 0;

  for (auto &constraint : problem.constraints) {
    error += constraintError(constraint);
  }

  return error;
}
#endif


static void changingCallback(Scene &scene,SceneSetup &setup)
{
  // The mouse button is down.  The scene is being changed, but we don't
  // consider this change complete.  We'll update the lines to be drawn
  // between the new positions.  We'll wait to update the box position
  // until the change is complete.

  updateLines(scene,setup);
#if 0
  printError();
#endif
}


static void changedCallback(Scene &scene,SceneSetup &setup)
{
#if !USE_SOLVER
  // We want to solve the box position at this point.
  Scene::Vector x(1,0,0);
  Scene::Vector y(0,1,0);
  Scene::Vector z(0,0,1);
  scene.setCoordinateAxes(setup.box, x, y, z);
#else
  solveBoxPosition(setup, box_global_transform);
#endif

  updateLines(scene,setup);
}


int main(int argc,char** argv)
{
  QApplication app(argc,argv);
  QMainWindow main_window;
  main_window.resize(640,480);
  main_window.show();
  OSGScene scene;

  SceneSetup scene_setup = setupScene(scene);
  scene.changed_callback = [&]{ changedCallback(scene,scene_setup); };
  scene.changing_callback = [&]{ changingCallback(scene,scene_setup); };

  GraphicsWindowPtr graphics_window_ptr =
    scene.createGraphicsWindow(ViewType::free);

  main_window.setCentralWidget(graphics_window_ptr->getGLWidget());
  app.exec();
}
