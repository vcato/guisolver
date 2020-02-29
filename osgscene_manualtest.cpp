#include <QMainWindow>
#include <QApplication>
#include <sstream>
#include "osgscene.hpp"
#include "readobj.hpp"

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

  Mesh mesh;
  std::istringstream stream(cube_obj_text);
  ObjData obj = readObj(stream);

  for (auto &v : obj.vertices) {
    mesh.vertices.push_back({v.x,v.y,v.z});
  }

  for (auto &face : obj.faces) {
    auto &vertex_indices = face.vertex_indices;
    assert(vertex_indices.size() >= 2);
    auto iter = vertex_indices.begin() + 2;
    auto v1 = vertex_indices[0] - 1;
    auto v3 = vertex_indices[1] - 1;

    for (; iter != vertex_indices.end(); ++iter) {
      auto v2 = v3;
      v3 = *iter - 1;
      mesh.triangles.push_back({v1,v2,v3});
    }
  }

  scene.createMesh(scene.top(), mesh);
  main_window.setCentralWidget(graphics_window_ptr->getGLWidget());
  main_window.resize(640,480);
  main_window.show();
  application.exec();
}
