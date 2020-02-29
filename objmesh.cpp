#include "objmesh.hpp"

#include <cmath>
#include <cassert>
#include "indicesof.hpp"


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


Mesh meshFromObj(const ObjData &obj)
{
  Mesh mesh;

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

  return mesh;
}
