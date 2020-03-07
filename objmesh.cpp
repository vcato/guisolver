#include "objmesh.hpp"

#include <cmath>
#include <cassert>
#include "indicesof.hpp"
#include "facenormalcalculator.hpp"



static Vec3
faceNormal(const ObjData::Face &face, const ObjData::Vertices &vertices)
{
  FaceNormalCalculator calculator;
  auto &vertex_indices = face.vertex_indices;
  auto n_face_vertices = vertex_indices.size();
  auto vec3 = [](const ObjData::Vertex &v){ return Vec3{v.x, v.y, v.z}; };

  for (auto i : indicesOf(vertex_indices)) {
    Vec3 v1 = vec3(vertices[vertex_indices[i] - 1]);
    Vec3 v2 = vec3(vertices[vertex_indices[(i+1)%n_face_vertices] - 1]);
    Vec3 v3 = vec3(vertices[vertex_indices[(i+2)%n_face_vertices] - 1]);
    calculator.addTriangle(v1, v2, v3);
  }

  return calculator.result();
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
