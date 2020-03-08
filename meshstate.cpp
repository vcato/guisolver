#include "meshstate.hpp"

#include "transformstate.hpp"
#include "facenormalcalculator.hpp"
#include "emplaceinto.hpp"

using std::cerr;


SceneState::MeshShape meshShapeStateFromMesh(const Mesh &mesh)
{
  SceneState::MeshShape mesh_shape;
  mesh_shape.positions.resize(mesh.positions.size());

  // Add positions
  for (auto index : indicesOf(mesh.positions)) {
    mesh_shape.positions[index] = xyzStateFromVec3(mesh.positions[index]);
  }

  // Add triangles
  for (auto index : indicesOf(mesh.triangles)) {
    Mesh::PositionIndex v1 = mesh.triangles[index].v1.position_index;
    Mesh::PositionIndex v2 = mesh.triangles[index].v2.position_index;
    Mesh::PositionIndex v3 = mesh.triangles[index].v3.position_index;
    SceneState::MeshShape::Triangle triangle_state(v1,v2,v3);
    emplaceInto(mesh_shape.triangles, index, triangle_state);
  }

  return mesh_shape;
}


static Vec3
triangleNormal(
  const SceneState::MeshShape::Triangle &triangle,
  const SceneState::MeshShape::Positions &positions
)
{
  FaceNormalCalculator calculator;
  Vec3 v1 = vec3FromXYZState(positions[triangle.v1]);
  Vec3 v2 = vec3FromXYZState(positions[triangle.v2]);
  Vec3 v3 = vec3FromXYZState(positions[triangle.v3]);
  calculator.addTriangle(v1, v2, v3);
  return calculator.result();
}


Mesh meshFromMeshShapeState(const SceneState::MeshShape &mesh_shape)
{
  Mesh mesh;

  // Add positions
  mesh.positions.resize(mesh_shape.positions.size(), Vec3{0,0,0});

  for (auto index : indicesOf(mesh_shape.positions)) {
    mesh.positions[index] = vec3FromXYZState(mesh_shape.positions[index]);
  }

  // Calculate normals.
  for (auto index : indicesOf(mesh_shape.triangles)) {
    Vec3 normal =
      triangleNormal(mesh_shape.triangles[index], mesh_shape.positions);

    emplaceInto(mesh.normals, index, normal);
  }

  // Add triangles
  for (auto triangle_index : indicesOf(mesh_shape.triangles)) {
    Mesh::PositionIndex p1 = mesh_shape.triangles[triangle_index].v1;
    Mesh::PositionIndex p2 = mesh_shape.triangles[triangle_index].v2;
    Mesh::PositionIndex p3 = mesh_shape.triangles[triangle_index].v3;
    Mesh::NormalIndex n1 = triangle_index;
    Mesh::NormalIndex n2 = triangle_index;
    Mesh::NormalIndex n3 = triangle_index;
    Mesh::Vertex v1{p1,n1};
    Mesh::Vertex v2{p2,n2};
    Mesh::Vertex v3{p3,n3};
    emplaceInto(mesh.triangles, triangle_index, v1, v2, v3);
  }

  return mesh;
}
