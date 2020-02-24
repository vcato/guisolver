#include <QMainWindow>
#include <QApplication>
#include "osgscene.hpp"


int main(int argc, char **argv)
{
  QApplication application(argc, argv);
  QMainWindow main_window;

  OSGScene scene;

  GraphicsWindowPtr graphics_window_ptr =
    scene.createGraphicsWindow(ViewType::free);

  Mesh mesh;
  mesh.vertices.push_back({0,0,0});
  mesh.vertices.push_back({1,0,0});
  mesh.vertices.push_back({0,1,0});
  mesh.vertices.push_back({1,1,0});
  mesh.triangles.push_back({0,1,2});
  mesh.triangles.push_back({2,1,3});
  scene.createMesh(scene.top(), mesh);
  main_window.setCentralWidget(graphics_window_ptr->getGLWidget());
  main_window.resize(640,480);
  main_window.show();
  application.exec();
}
