#include <QApplication>
#include <QMainWindow>
#include "osgscene.hpp"
#include "setupscene.hpp"


int main(int argc,char** argv)
{
  QApplication app(argc,argv);
  QMainWindow main_window;
  main_window.resize(640,480);
  main_window.show();
  OSGScene scene;

  setupScene(scene);

  GraphicsWindowPtr graphics_window_ptr =
    scene.createGraphicsWindow(ViewType::free);

  main_window.setCentralWidget(graphics_window_ptr->getGLWidget());
  app.exec();
}
