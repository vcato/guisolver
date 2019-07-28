#include <iostream>
#include <QApplication>
#include <QMainWindow>
#include "osgscene.hpp"
#include "setupscene.hpp"


using std::cerr;


static void changingCallback(Scene &scene,SceneSetup &setup)
{
  for (auto &line : setup.lines) {
    Scene::Point start = scene.worldPoint({0,0,0}, line.start);
    Scene::Point end = scene.worldPoint({0,0,0}, line.end);
    scene.setStartPoint(line.handle,start);
    scene.setEndPoint(line.handle,end);
  }
}


int main(int argc,char** argv)
{
  QApplication app(argc,argv);
  QMainWindow main_window;
  main_window.resize(640,480);
  main_window.show();
  OSGScene scene;

  SceneSetup scene_setup = setupScene(scene);
  // scene.change_callback = [&]{ changedCallback(scene,scene_setup); };
  scene.changing_callback = [&]{ changingCallback(scene,scene_setup); };

  GraphicsWindowPtr graphics_window_ptr =
    scene.createGraphicsWindow(ViewType::free);

  main_window.setCentralWidget(graphics_window_ptr->getGLWidget());
  app.exec();
}
