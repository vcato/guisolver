#include <QMainWindow>
#include <QApplication>
#include <sstream>
#include "osgscene.hpp"
#include "readobj.hpp"
#include "indicesof.hpp"
#include "objmesh.hpp"


using std::string;
using std::cerr;


static const char *cube_obj_text =
  "#Name:Cube\n"
  "#Type:Face-specified\n"
  "#Direction:Clockwise\n"
  "\n"
  "v   -0.5    -0.5    -0.5\n"
  "v   -0.5    -0.5    0.5\n"
  "v   -0.5    0.5    -0.5\n"
  "v   -0.5    0.5    0.5\n"
  "v   0.5    -0.5    -0.5\n"
  "v   0.5    -0.5    0.5\n"
  "v   0.5    0.5    -0.5\n"
  "v   0.5    0.5    0.5\n"
  "\n"
  "f   8    4    2    6\n"
  "f   8    6    5    7\n"
  "f   8    7    3    4\n"
  "f   4    3    1    2\n"
  "f   1    3    7    5\n"
  "f   2    1    5    6\n"
;



int main(int argc, char **argv)
{
  QApplication application(argc, argv);
  QMainWindow main_window;
  OSGScene scene;

  GraphicsWindowPtr graphics_window_ptr =
    scene.createGraphicsWindow(ViewType::free);

  {
    std::istringstream stream(cube_obj_text);
    ObjData obj = readObj(stream);
    Mesh mesh = meshFromObj(obj);
    scene.createMesh(scene.top(), mesh);
  }

  Scene::GeometryHandle box_handle = scene.createBox(scene.top());
  scene.setGeometryCenter(box_handle, {1,0,0});
  main_window.setCentralWidget(graphics_window_ptr->getGLWidget());
  main_window.resize(640,480);
  main_window.show();
  application.exec();
}
