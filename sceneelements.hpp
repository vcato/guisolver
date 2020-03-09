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


struct MarkerPosition {
  Marker marker;
};


struct DistanceError {
  DistanceErrorIndex index;
  DistanceError(DistanceErrorIndex index) : index(index) {}
};


struct DistanceErrorDesiredDistance {
  DistanceError distance_error;
};


struct DistanceErrorWeight {
  DistanceError distance_error;
};


struct Variable {
  VariableIndex index;
  Variable(VariableIndex index) : index(index) {}
};


struct VariableValue {
  Variable variable;
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


struct BodyLineStart {
  BodyLine body_line;
};


struct BodyLineEnd {
  BodyLine body_line;
};


struct BodyMesh {
  Body body;
  MeshIndex index;
  BodyMesh(Body body, MeshIndex index) : body(body), index(index) {}
};


struct BodyMeshPositions {
  BodyMesh body_mesh;
};


struct BodyMeshScale {
  BodyMesh body_mesh;
};


struct BodyMeshCenter {
  BodyMesh body_mesh;
};


template <typename Array, typename Index>
struct ArrayElement {
  Array array;
  Index index;
};

template <typename Parent>
struct ElementXYZComponent {
  Parent parent;
  XYZComponent component;
};


using BodyMeshPosition = ArrayElement<BodyMeshPositions, size_t>;
using BodyMeshPositionComponent = ElementXYZComponent<BodyMeshPosition>;
using BodyMeshScaleComponent = ElementXYZComponent<BodyMeshScale>;
using BodyMeshCenterComponent = ElementXYZComponent<BodyMeshCenter>;
