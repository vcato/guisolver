#ifndef OBJDATA_HPP_
#define OBJDATA_HPP_

#include "vector.hpp"


struct ObjData {
  struct Vertex;
  struct Face;
  using VertexIndex = int;

  struct Vertex {
    float x,y,z;
  };

  struct Face {
    vector<VertexIndex> vertex_indices;
  };

  using Vertices = vector<Vertex>;
  Vertices vertices;
  using Faces = vector<Face>;
  Faces faces;
};


#endif /* OBJDATA_HPP_ */
