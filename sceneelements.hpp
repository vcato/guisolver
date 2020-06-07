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


template <typename Array, typename Index>
struct ArrayElement {
  Array array;
  Index index;

  ArrayElement(Array array, Index index)
  : array(array), index(index)
  {
  }

  bool operator==(const ArrayElement &arg) const
  {
    return array == arg.array && index == arg.index;
  }
};


struct Body {
  struct Mesh;
  BodyIndex index;
  Body(BodyIndex index) : index(index) {}

  bool operator==(const Body &arg) const
  {
    return index == arg.index;
  }

  inline Mesh mesh(MeshIndex mesh_index);
};


struct Body::Mesh {
  struct Positions;
  Body body;
  MeshIndex index;

  bool operator==(const Mesh &arg) const
  {
    return body==arg.body && index==arg.index;
  }

  inline Positions positions() const;

  inline ArrayElement<Positions,MeshPositionIndex> position(MeshPositionIndex i);

  private:
  friend class Body;
  Mesh(Body body, MeshIndex index) : body(body), index(index) {}
};


using BodyMesh = Body::Mesh;

inline Body::Mesh Body::mesh(MeshIndex mesh_index)
{
  return Mesh(*this, mesh_index);
}


using BodyMeshPositions = BodyMesh::Positions;
using BodyMeshPosition = ArrayElement<BodyMeshPositions, MeshPositionIndex>;


struct BodyMesh::Positions {
  BodyMesh body_mesh;

  bool operator==(const Positions &arg) const
  {
    return body_mesh == arg.body_mesh;
  }

  BodyMeshPosition operator[](MeshPositionIndex i)
  {
    return BodyMeshPosition{*this, i};
  }

  private:
    friend class Body::Mesh;
    Positions(BodyMesh body_mesh) : body_mesh(body_mesh) {}
};


BodyMeshPosition Body::Mesh::position(MeshPositionIndex i)
{
  return {*this, i};
}


inline BodyMesh::Positions BodyMesh::positions() const
{
  return BodyMeshPositions(*this);
}


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

  bool operator==(const Marker &arg) const
  {
    return index == arg.index;
  }
};


inline Optional<MarkerIndex> makeMarkerIndex(Optional<Marker> maybe_marker)
{
  if (!maybe_marker) {
    return {};
  }

  return maybe_marker->index;
}


inline Optional<Marker> makeMarker(Optional<MarkerIndex> arg)
{
  if (arg) {
    return Marker(*arg);
  }
  else {
    return {};
  }
}


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


struct BodyMeshScale {
  BodyMesh body_mesh;
};


struct BodyMeshCenter {
  BodyMesh body_mesh;
};


using BodyMeshPositionComponent = ElementXYZComponent<BodyMeshPosition>;
using BodyMeshScaleComponent = ElementXYZComponent<BodyMeshScale>;
using BodyMeshCenterComponent = ElementXYZComponent<BodyMeshCenter>;

#endif /* SCENEELEMENTS_HPP_ */
