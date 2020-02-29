#include <QMainWindow>
#include <QApplication>
#include <sstream>
#include "osgscene.hpp"
#include "readobj.hpp"
#include "indicesof.hpp"

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



static Vec3
crossProduct(const Vec3 &a, const Vec3 &b)
{
  auto x = a.y*b.z - a.z*b.y;
  auto y = a.z*b.x - a.x*b.z;
  auto z = a.x*b.y - a.y*b.x;
  return {x,y,z};
}


static Vec3::Scalar dot(const Vec3 &a, const Vec3 &b)
{
  return a.x*b.x + a.y*b.y + a.z*b.z;
}


static Vec3::Scalar magnitude(const Vec3 &arg)
{
  using std::sqrt;
  return sqrt(dot(arg, arg));
}


static Vec3 normalized(const Vec3 &arg)
{
  return arg/magnitude(arg);
}


static Vec3
faceNormal(const ObjData::Face &face, const ObjData::Vertices &vertices)
{
  Vec3 total{0,0,0};
  auto &vertex_indices = face.vertex_indices;
  auto n_face_vertices = vertex_indices.size();
  auto vec3 = [](const ObjData::Vertex &v){ return Vec3{v.x, v.y, v.z}; };

  for (auto i : indicesOf(vertex_indices)) {
    auto v1 = vec3(vertices[vertex_indices[i] - 1]);
    auto v2 = vec3(vertices[vertex_indices[(i+1)%n_face_vertices] - 1]);
    auto v3 = vec3(vertices[vertex_indices[(i+2)%n_face_vertices] - 1]);
    total += crossProduct(v2-v1, v3-v2);
  }

  return normalized(total);
}



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

  // Create normals

  vector<Vec3> face_normals;

  for (auto &face : obj.faces) {
    face_normals.push_back(faceNormal(face, obj.vertices));
  }

  using NormalIndex = Mesh::NormalIndex;
  using PositionIndex = Mesh::PositionIndex;

  for (auto &v : obj.vertices) {
    mesh.positions.push_back({v.x, v.y, v.z});
  }

  mesh.normals = face_normals;

  for (auto face_index : indicesOf(obj.faces)) {
    auto &face = obj.faces[face_index];
    auto &vertex_indices = face.vertex_indices;
    assert(vertex_indices.size() >= 2);
    auto iter = vertex_indices.begin() + 2;
    PositionIndex v1 = vertex_indices[0] - 1;
    NormalIndex n = face_index;
    PositionIndex v3 = vertex_indices[1] - 1;

    for (; iter != vertex_indices.end(); ++iter) {
      PositionIndex v2 = v3;
      v3 = *iter - 1;
      mesh.triangles.push_back({{v1,n},{v2,n},{v3,n}});
    }
  }

  scene.createMesh(scene.top(), mesh);
  Scene::GeometryHandle box_handle = scene.createBox(scene.top());
  scene.setGeometryCenter(box_handle, {1,0,0});
  main_window.setCentralWidget(graphics_window_ptr->getGLWidget());
  main_window.resize(640,480);
  main_window.show();
  application.exec();
}
