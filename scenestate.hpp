#ifndef SCENESTATE_HPP_
#define SCENESTATE_HPP_

#include "vector.hpp"
#include "markerindex.hpp"
#include "bodyindex.hpp"
#include "distanceerrorindex.hpp"
#include "optional.hpp"
#include "indicesof.hpp"


using BoxIndex = int;
using LineIndex = int;

class SceneState {
  public:
    struct Marker;
    struct DistanceError;
    struct Transform;
    struct XYZ;
    struct Body;
    using Markers = vector<Marker>;
    using Bodies = vector<Body>;
    using DistanceErrors = vector<DistanceError>;
    using String = std::string;
    using Position = XYZ;

    struct XYZ {
      float x = 0;
      float y = 0;
      float z = 0;
    };

    struct Marker {
      using Name = String;
      Position position;
      Optional<BodyIndex> maybe_body_index;
      Name name;
    };

    struct Transform {
      XYZ translation;
      XYZ rotation;
    };

    struct XYZSolveFlags {
      bool x = false;
      bool y = false;
      bool z = false;
    };

    struct TransformSolveFlags {
      XYZSolveFlags translation;
      XYZSolveFlags rotation;
    };

    struct Box {
      XYZ scale = {1,1,1};
      Position center = {0,0,0};
    };

    struct Line {
      Position start = {0,0,0};
      Position end = {1,0,0};
    };

    struct Body {
      using Name = String;
      Name name;
      Transform transform;
      vector<Box> boxes;
      vector<Line> lines;
      TransformSolveFlags solve_flags;
      Optional<BodyIndex> maybe_parent_index;

      BoxIndex addBox()
      {
        BoxIndex box_index = boxes.size();
        boxes.emplace_back();
        return box_index;
      }

      LineIndex addLine()
      {
        LineIndex line_index = lines.size();
        lines.emplace_back();
        return line_index;
      }

      Body(const Name &name) : name(name) {}
    };

    struct DistanceError {
      Optional<MarkerIndex> optional_start_marker_index;
      Optional<MarkerIndex> optional_end_marker_index;
      Optional<float> maybe_distance;
      float desired_distance = 0;
      float weight = 1;
      float error = 0;
    };

    DistanceErrors distance_errors;
    float total_error = 0;

    SceneState();

    const Markers &markers() const { return _markers; }
    const Bodies &bodies() const { return _bodies; }
    Marker &marker(MarkerIndex index) { return _markers[index]; }
    const Marker &marker(MarkerIndex index) const { return _markers[index]; }
    Body &body(BodyIndex index) { return _bodies[index]; }
    const Body &body(BodyIndex index) const { return _bodies[index]; }

    MarkerIndex createMarker(Optional<BodyIndex>);

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

    void removeMarker(MarkerIndex index_to_remove);

    bool bodyHasChildren(BodyIndex body_index) const;

    void removeBody(BodyIndex index_to_remove);

    DistanceErrorIndex createDistanceError()
    {
      DistanceErrorIndex index = distance_errors.size();
      distance_errors.emplace_back();
      return index;
    }

    void removeDistanceError(int index);

  private:
    Markers _markers;
    Bodies _bodies;

    void
      _handleMarkerRemoved(
        Optional<MarkerIndex> &optional_marker_index,
        MarkerIndex index_to_remove
      )
    {
      if (!optional_marker_index) return;

      MarkerIndex &marker_index = *optional_marker_index;

      if (marker_index == index_to_remove) {
        optional_marker_index.reset();
      }
      else if (marker_index > index_to_remove) {
        --marker_index;
      }
    }
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
  for (auto marker_index : indicesOf(scene_state.markers())) {
    if (scene_state.marker(marker_index).maybe_body_index == body_index) {
      marker_indices.push_back(marker_index);
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


inline void
preOrderTraverseBodyBranch(
  BodyIndex body_index,
  const SceneState &scene_state,
  vector<BodyIndex> &body_indices
)
{
  body_indices.push_back(body_index);

  for (BodyIndex other_body_index : indicesOf(scene_state.bodies())) {
    if (scene_state.body(other_body_index).maybe_parent_index == body_index) {
      assert(other_body_index != body_index);
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
  BodyIndex body_index,
  const SceneState &scene_state,
  const Visitor &visitor
)
{
  vector<BodyIndex> body_indices;
  preOrderTraverseBodyBranch(body_index, scene_state, body_indices);

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
  findMarkerIndex(const SceneState &, const SceneState::Marker::Name &);

extern Optional<MarkerIndex>
  findBodyIndex(const SceneState &, const SceneState::Body::Name &);


#endif /* SCENESTATE_HPP_ */
