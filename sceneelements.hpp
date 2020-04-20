#ifndef SCENEELEMENTS_HPP_
#define SCENEELEMENTS_HPP_

#include <cstdlib>
#include "bodyindex.hpp"
#include "lineindex.hpp"
#include "meshindex.hpp"
#include "boxindex.hpp"
#include "distanceerrorindex.hpp"
#include "variableindex.hpp"
#include "markerindex.hpp"
#include "xyzcomponent.hpp"


struct Body {
  BodyIndex index;
  Body(BodyIndex index) : index(index) {}
};


struct BodyXYZComponent {
  Body body;
  XYZComponent component;

  BodyXYZComponent(Body body, XYZComponent component)
  : body(body),
    component(component)
  {
  }
};


template <typename Parent>
struct ElementXYZComponent {
  Parent parent;
  XYZComponent component;
};


struct BodyTranslation {
  Body body;
};


using BodyTranslationComponent = ElementXYZComponent<BodyTranslation>;


struct BodyRotation {
  Body body;
};


using BodyRotationComponent = ElementXYZComponent<BodyRotation>;


struct BodyScale {
  Body body;
};


struct BodyBox {
  Body body;
  BoxIndex index;
  BodyBox(Body body, BoxIndex index) : body(body), index(index) {}
};


struct BodyBoxScale {
  BodyBox body_box;
};


using BodyBoxScaleComponent = ElementXYZComponent<BodyBoxScale>;


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

using BodyMeshPosition = ArrayElement<BodyMeshPositions, size_t>;
using BodyMeshPositionComponent = ElementXYZComponent<BodyMeshPosition>;
using BodyMeshScaleComponent = ElementXYZComponent<BodyMeshScale>;
using BodyMeshCenterComponent = ElementXYZComponent<BodyMeshCenter>;

#endif /* SCENEELEMENTS_HPP_ */
