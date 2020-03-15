#ifndef MESH_HPP_
#define MESH_HPP_

#include "vector.hpp"
#include "vec3.hpp"


struct Mesh {
  struct Triangle;
  using PositionIndex = int;
  using NormalIndex = int;
  using Positions = vector<Vec3>;
  using Triangles = vector<Triangle>;

  struct Vertex {
    PositionIndex position_index;
    NormalIndex normal_index;

    Vertex(PositionIndex position_index, NormalIndex normal_index)
    : position_index(position_index), normal_index(normal_index)
    {
    }
  };

  struct Triangle {
    Vertex v1, v2, v3;

    Triangle(Vertex v1, Vertex v2, Vertex v3)
    : v1(v1), v2(v2), v3(v3)
    {
    }
  };

  Positions positions;
  vector<Vec3> normals;
  Triangles triangles;
};


#endif /* MESH_HPP_ */
