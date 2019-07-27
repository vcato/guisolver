#include <QApplication>
#include <QMainWindow>
#include "osgscenemanager.hpp"
#include "setupscene.hpp"


int main(int argc,char** argv)
{
  QApplication app(argc,argv);
  QMainWindow main_window;
  main_window.resize(640,480);
  main_window.show();
  OSGSceneManager scene_manager;
  scene_manager.selection_handler.use_screen_relative_dragger = true;

  setupScene(scene_manager.scene);

  GraphicsWindowPtr graphics_window_ptr =
    scene_manager.createGraphicsWindow(ViewType::free);

  main_window.setCentralWidget(graphics_window_ptr->getGLWidget());
  app.exec();
}
