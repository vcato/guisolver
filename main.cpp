#include <QApplication>
#include <QMainWindow>
#include "osgscenemanager.hpp"


int main(int argc,char** argv)
{
  QApplication app(argc,argv);
  QMainWindow main_window;
  main_window.resize(640,480);
  main_window.show();
  OSGSceneManager scene_manager;
  scene_manager.scene.create();
  GraphicsWindowPtr graphics_window_ptr =
    scene_manager.createGraphicsWindow(ViewType::free);
  main_window.setCentralWidget(graphics_window_ptr->getGLWidget());

#if 0
  {
    osgViewer::GraphicsWindow::Views views;
    graphics_window_ptr->getViews(views);
    views.front()->setSceneData(scene_manager.scene.top_node_ptr);
  }
#endif

  app.exec();
}
