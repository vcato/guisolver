#include "bodyindex.hpp"
#include "lineindex.hpp"
#include "meshindex.hpp"


struct Body {
  BodyIndex index;
  Body(BodyIndex index) : index(index) {}
};


struct Marker {
  MarkerIndex index;
  Marker(MarkerIndex index) : index(index) {}
};


struct DistanceError {
  DistanceErrorIndex index;
  DistanceError(DistanceErrorIndex index) : index(index) {}
};


struct Variable {
  VariableIndex index;
  Variable(VariableIndex index) : index(index) {}
};


struct BodyBox {
  Body body;
  BoxIndex index;
  BodyBox(Body body, BoxIndex index) : body(body), index(index) {}
};


struct BodyLine {
  Body body;
  LineIndex index;
  BodyLine(Body body, LineIndex index) : body(body), index(index) {}
};


struct BodyMesh {
  Body body;
  MeshIndex index;
  BodyMesh(Body body, MeshIndex index) : body(body), index(index) {}
};
