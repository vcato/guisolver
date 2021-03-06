#ifndef SCENESTATE_HPP_
#define SCENESTATE_HPP_

#include "vector.hpp"
#include "markerindex.hpp"
#include "bodyindex.hpp"
#include "distanceerrorindex.hpp"
#include "variableindex.hpp"
#include "optional.hpp"
#include "indicesof.hpp"
#include "xyzcomponent.hpp"
#include "variablename.hpp"
#include "matchconst.hpp"
#include "channel.hpp"
#include "boxindex.hpp"
#include "lineindex.hpp"
#include "mesh.hpp"
#include "pointlink.hpp"

using Expression = std::string;


template <typename XYZ>
auto &
component(XYZ &self, XYZComponent component)
{
  switch (component) {
    case XYZComponent::x: return self.x;
    case XYZComponent::y: return self.y;
    case XYZComponent::z: return self.z;
  }

  assert(false);
  return self.x;
}


class SceneState {
  public:
    struct Marker;
    struct DistanceError;
    struct Transform;
    struct XYZ;
    struct Body;
    struct Variable;
    using Markers = vector<Marker>;
    using Bodies = vector<Body>;
    using DistanceErrors = vector<DistanceError>;
    using Variables = vector<Variable>;
    using String = std::string;
    using Position = XYZ;
    using Expression = ::Expression;
    using Float = float;

    struct XYZ {
      Float x = 0;
      Float y = 0;
      Float z = 0;

      Float &component(XYZComponent component)
      {
        return ::component(*this, component);
      }

      const Float &component(XYZComponent component) const
      {
        return ::component(*this, component);
      }

      friend std::ostream& operator<<(std::ostream &stream, const XYZ &arg)
      {
        stream << "SceneState::XYZ";
        stream << "(" << arg.x << "," << arg.y << "," << arg.z << ")";

        return stream;
      }
    };

    struct XYZExpressions {
      Expression x;
      Expression y;
      Expression z;
    };

    struct XYZSolveFlags {
      bool x = false;
      bool y = false;
      bool z = false;

      bool component(XYZComponent component) const
      {
        switch (component) {
          case XYZComponent::x: return x;
          case XYZComponent::y: return y;
          case XYZComponent::z: return z;
        }

        assert(false);
        return false;
      }
    };

    struct XYZChannelsRef {
      const XYZ &values;
      const XYZExpressions &expressions;
    };

    struct Marker {
      using Name = String;
      Position position;
      XYZExpressions position_expressions;
      Optional<BodyIndex> maybe_body_index;
      Name name;

      XYZChannelsRef positionChannels() const
      {
        return XYZChannelsRef{position, position_expressions};
      }
    };

    struct Transform {
      static Float defaultScale() { return 1; }

      XYZ translation;
      XYZ rotation;
      Float scale = defaultScale();
    };

    struct TransformSolveFlags {
      static bool defaultScale() { return false; }
      XYZSolveFlags translation;
      XYZSolveFlags rotation;
      bool scale = defaultScale();
    };

    struct TransformExpressions {
      static Expression defaultScale() { return ""; }
      XYZExpressions translation;
      XYZExpressions rotation;
      Expression scale;
    };

    struct Box {
      XYZ scale = {1,1,1};
      Position center = {0,0,0};
      XYZExpressions scale_expressions;
      XYZExpressions center_expressions;

      XYZChannelsRef scaleChannels() const
      {
        return XYZChannelsRef{scale, scale_expressions};
      }

      XYZChannelsRef centerChannels() const
      {
        return XYZChannelsRef{center, center_expressions};
      }
    };

    struct Line {
      Position start = {0,0,0};
      Position end = {1,0,0};
    };

    struct MeshShape {
      struct Triangle;
      using Positions = vector<XYZ>;
      using Triangles = vector<Triangle>;

      struct Triangle {
        int v1, v2, v3;
        Triangle(int v1, int v2, int v3)
        : v1(v1), v2(v2), v3(v3)
        {
        }
      };

      Positions positions;
      Triangles triangles;
    };

    struct Mesh {
      MeshShape shape;
      XYZ scale = {1,1,1};
      Position center = {0,0,0};
      XYZSolveFlags scale_solve_flags;
      XYZExpressions scale_expressions;
    };

    struct Body {
      using Name = String;
      Name name;
      Transform transform;
      vector<Box> boxes;
      vector<Line> lines;
      vector<Mesh> meshes;
      TransformSolveFlags solve_flags;
      TransformExpressions expressions;
      Optional<BodyIndex> maybe_parent_index;

      Body(const Name &name) : name(name) {}

      BoxIndex createBox()
      {
        BoxIndex box_index = boxes.size();
        boxes.emplace_back();
        return box_index;
      }

      LineIndex createLine()
      {
        LineIndex line_index = lines.size();
        lines.emplace_back();
        return line_index;
      }

      MeshIndex createMesh(const MeshShape &);
    };

    struct DistanceError {
      Optional<PointLink> optional_start;
      Optional<PointLink> optional_end;
      Optional<BodyIndex> maybe_body_index;
      Optional<Float> maybe_distance;
      Float desired_distance = 0;
      Float weight = 1;
      Float error = 0;

      void setStart(Optional<PointLink> arg)
      {
        optional_start = arg;
      }

      void setStart(Optional<::Marker> arg)
      {
        setStart(maybePointLink(arg));
      }

      void setStart(Optional<BodyMeshPosition> arg)
      {
        setStart(maybePointLink(arg));
      }

      void setEnd(Optional<PointLink> arg)
      {
        optional_end = arg;
      }

      void setEnd(Optional<::Marker> arg)
      {
        setEnd(maybePointLink(arg));
      }

      void setEnd(Optional<BodyMeshPosition> arg)
      {
        setEnd(maybePointLink(arg));
      }

      bool hasStart() const { return optional_start.hasValue(); }
      bool hasEnd() const { return optional_end.hasValue(); }
    };

    struct Variable {
      using Name = VariableName;
      Name name;
      Float value = 0;
    };

    DistanceErrors distance_errors;
    Variables variables;
    Float total_error = 0;
    Optional<BodyMeshPosition> maybe_marked_body_mesh_position;
    Optional<::Marker> maybe_marked_marker;

    SceneState();

    const Markers &markers() const { return _markers; }
    const Bodies &bodies() const { return _bodies; }
    Marker &marker(MarkerIndex index) { return _markers[index]; }
    const Marker &marker(MarkerIndex index) const { return _markers[index]; }
    Body &body(BodyIndex index) { return _bodies[index]; }
    const Body &body(BodyIndex index) const { return _bodies[index]; }

    Marker &operator[](::Marker marker) { return this->marker(marker.index); }

    MarkerIndex createMarker(Optional<BodyIndex> = {});

    MarkerIndex createMarker(const String &name)
    {
      MarkerIndex new_index = _markers.size();
      _markers.emplace_back();
      _markers.back().name = name;
      return new_index;
    }

    MarkerIndex duplicateMarker(MarkerIndex);

    BodyIndex createBody(Optional<BodyIndex> maybe_parent_index = {});

    MarkerIndex createUnnamedMarker()
    {
      return createMarker("");
    }

    bool bodyHasChildren(BodyIndex body_index) const;

    void removeMarker(MarkerIndex);
    void removeBody(BodyIndex);
    void removeDistanceError(DistanceErrorIndex index);
    void removeVariable(VariableIndex);

    DistanceErrorIndex
    createDistanceError(Optional<BodyIndex> maybe_body_index = {})
    {
      DistanceErrorIndex index = distance_errors.size();
      distance_errors.emplace_back();
      distance_errors[index].maybe_body_index = maybe_body_index;
      return index;
    }

    VariableIndex createVariable();

  private:
    Markers _markers;
    Bodies _bodies;

    void
      _handleMarkerRemoved(
        Optional<PointLink> &optional_point_link,
        MarkerIndex index_to_remove
      );
};


using TransformState = SceneState::Transform;
using TranslationState = SceneState::XYZ;
using RotationState = SceneState::XYZ;
using PositionState = SceneState::Position;

extern vector<SceneState::Marker::Name> markerNames(const SceneState &state);
extern vector<SceneState::Body::Name> bodyNames(const SceneState &state);

inline void setAll(SceneState::XYZSolveFlags &flags, bool value)
{
  flags.x = value;
  flags.y = value;
  flags.z = value;
}

inline void setAll(SceneState::TransformSolveFlags &flags, bool state)
{
  setAll(flags.translation, state);
  setAll(flags.rotation, state);
}

extern vector<MarkerIndex>
  indicesOfMarkersOnBody(
    Optional<BodyIndex> maybe_body_index,
    const SceneState &scene_state
  );

extern vector<DistanceErrorIndex>
  indicesOfDistanceErrorsOnBody(
    Optional<BodyIndex> maybe_body_index,
    const SceneState &scene_state
  );

extern vector<BodyIndex>
  indicesOfChildBodies(
    Optional<BodyIndex> maybe_body_index,
    const SceneState &scene_state
  );


inline void
addMarkersOnBodyTo(
  vector<MarkerIndex> &marker_indices,
  BodyIndex body_index,
  const SceneState &scene_state
)
{
  for (auto i : indicesOf(scene_state.markers())) {
    if (scene_state.marker(i).maybe_body_index == body_index) {
      marker_indices.push_back(i);
    }
  }
}


inline void
addDistanceErrorsOnBodyTo(
  vector<DistanceErrorIndex> &distance_error_indices,
  BodyIndex body_index,
  const SceneState &scene_state
)
{
  for (auto i : indicesOf(scene_state.distance_errors)) {
    if (scene_state.distance_errors[i].maybe_body_index == body_index) {
      distance_error_indices.push_back(i);
    }
  }
}


inline vector<MarkerIndex>
markersOnBody(BodyIndex body_index, const SceneState &scene_state)
{
  vector<MarkerIndex> indices;
  addMarkersOnBodyTo(indices, body_index, scene_state);
  return indices;
}


inline vector<DistanceErrorIndex>
distanceErrorsOnBody(BodyIndex body_index, const SceneState &scene_state)
{
  vector<DistanceErrorIndex> indices;
  addDistanceErrorsOnBodyTo(indices, body_index, scene_state);
  return indices;
}


inline void
preOrderTraverseBodyBranch(
  Optional<BodyIndex> maybe_branch_body_index,
  const SceneState &scene_state,
  vector<BodyIndex> &body_indices
)
{
  if (maybe_branch_body_index) {
    body_indices.push_back(*maybe_branch_body_index);
  }

  for (BodyIndex other_body_index : indicesOf(scene_state.bodies())) {
    if (
      scene_state.body(other_body_index).maybe_parent_index
      == maybe_branch_body_index
    ) {
      assert(maybe_branch_body_index != other_body_index);
      preOrderTraverseBodyBranch(other_body_index, scene_state, body_indices);
    }
  }
}


inline void
postOrderTraverseBodyBranch(
  BodyIndex body_index,
  const SceneState &scene_state,
  vector<BodyIndex> &body_indices
)
{
  for (BodyIndex other_body_index : indicesOf(scene_state.bodies())) {
    if (scene_state.body(other_body_index).maybe_parent_index == body_index) {
      postOrderTraverseBodyBranch(other_body_index, scene_state, body_indices);
    }
  }

  body_indices.push_back(body_index);
}


template <typename Visitor>
inline void
forEachBranchIndexInPostOrder(
  BodyIndex body_index,
  const SceneState &scene_state,
  const Visitor &visitor
)
{
  vector<BodyIndex> body_indices;
  postOrderTraverseBodyBranch(body_index, scene_state, body_indices);

  for (auto body_index : body_indices) {
    for (auto marker_index : markersOnBody(body_index, scene_state)) {
      visitor.visitMarker(marker_index);
    }

    visitor.visitBody(body_index);
  }
}


template <typename Visitor>
inline void
forEachBranchIndexInPreOrder(
  Optional<BodyIndex> maybe_branch_body_index,
  const SceneState &scene_state,
  const Visitor &visitor
)
{
  vector<BodyIndex> body_indices;
  preOrderTraverseBodyBranch(maybe_branch_body_index, scene_state, body_indices);

  for (auto body_index : body_indices) {
    visitor.visitBody(body_index);

    for (auto marker_index : markersOnBody(body_index, scene_state)) {
      visitor.visitMarker(marker_index);
    }
  }
}


extern bool
  hasAncestor(
    BodyIndex body_index,
    BodyIndex ancestor_body_index,
    const SceneState &state
  );


extern Optional<MarkerIndex>
  findMarkerWithName(const SceneState &, const SceneState::Marker::Name &);

extern Optional<MarkerIndex>
  findBodyWithName(const SceneState &, const SceneState::Body::Name &);

extern Optional<VariableIndex>
  findVariableWithName(const SceneState &, const SceneState::Variable::Name &);


inline TranslationState translationStateOf(const TransformState &arg)
{
  return arg.translation;
}


inline RotationState rotationStateOf(const TransformState &arg)
{
  return arg.rotation;
}


inline SceneState::Expression &
xyzExpressionsComponent(
  SceneState::XYZExpressions &xyz_expressions,
  XYZComponent xyz_component
)
{
  switch (xyz_component) {
    case XYZComponent::x: return xyz_expressions.x;
    case XYZComponent::y: return xyz_expressions.y;
    case XYZComponent::z: return xyz_expressions.z;
  }

  assert(false);
  return xyz_expressions.x;
}


extern SceneState::Expression &
  channelExpression(const Channel &channel, SceneState &scene_state);



template <typename Visitor>
inline void
forEachBodyGeometryType(const Visitor &visitor)
{
  visitor.visitBoxes();
  visitor.visitLines();
  visitor.visitMeshes();
}


template <typename GeometryVisitor>
inline void
forEachBodyGeometry(
  const SceneState::Body &body_state, GeometryVisitor &visitor
)
{
  struct TypeVisitor {
    const SceneState::Body &body_state;
    GeometryVisitor &visitor;

    void visitBoxes() const
    {
      for (auto i : indicesOf(body_state.boxes)) {
        visitor.visitBox(i);
      }
    }

    void visitLines() const
    {
      for (auto i : indicesOf(body_state.lines)) {
        visitor.visitLine(i);
      }
    }

    void visitMeshes() const
    {
      for (auto i : indicesOf(body_state.meshes)) {
        visitor.visitMesh(i);
      }
    }
  };

  TypeVisitor type_visitor{body_state, visitor};
  forEachBodyGeometryType(type_visitor);
}


extern SceneState::Float
  bodyGlobalScale(BodyIndex body_index, const SceneState &scene_state);

#endif /* SCENESTATE_HPP_ */
