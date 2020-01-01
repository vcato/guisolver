#ifndef SCENESTATE_HPP_
#define SCENESTATE_HPP_

#include "vector.hpp"
#include "markerindex.hpp"
#include "bodyindex.hpp"
#include "distanceerrorindex.hpp"
#include "vector.hpp"
#include "optional.hpp"
#include "removeindexfrom.hpp"
#include "indicesof.hpp"
#include "vectorio.hpp"


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

    struct XYZ {
      float x = 0;
      float y = 0;
      float z = 0;
    };

    struct Marker {
      using Name = String;
      XYZ position;
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

    struct Geometry {
      XYZ scale = {1,1,1};
      XYZ center = {0,0,0};
    };

    struct Body {
      Body() = default;
      Transform transform;
      Geometry geometry;
      TransformSolveFlags solve_flags;
      Optional<BodyIndex> maybe_parent_index;
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

    void removeMarker(MarkerIndex index_to_remove)
    {
      removeIndexFrom(_markers, index_to_remove);

      for (auto &distance_error : distance_errors) {
        _handleMarkerRemoved(
          distance_error.optional_start_marker_index, index_to_remove
        );

        _handleMarkerRemoved(
          distance_error.optional_end_marker_index, index_to_remove
        );
      }
    }

    bool bodyHasChildren(BodyIndex body_index) const;

    void removeBody(BodyIndex index_to_remove);

    DistanceErrorIndex createDistanceError()
    {
      DistanceErrorIndex index = distance_errors.size();
      distance_errors.emplace_back();
      return index;
    }

    void removeDistanceError(int index)
    {
      removeIndexFrom(distance_errors, index);
    }

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
using MarkerPosition = SceneState::XYZ;

extern vector<SceneState::Marker::Name> markerNames(const SceneState &state);

extern BodyIndex
  createBodyInState(SceneState &, Optional<BodyIndex> maybe_parent_index);

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


template <typename Index, typename Function>
void removeIndices(vector<Index> &indices, const Function &remove_function)
{
  size_t n = indices.size();

  for (size_t i=0; i!=n; ++i) {
    remove_function(indices[i]);

    for (size_t j=i+1; j!=n; ++j) {
      if (indices[j] > indices[i]) {
        --indices[j];
      }
    }
  }
}


template <typename Visitor>
extern void
removeMarkersOnBody(
  BodyIndex body_index,
  SceneState &scene_state,
  const Visitor &visitor
)
{
  vector<MarkerIndex> indices_of_markers_to_remove;
  addMarkersOnBodyTo(indices_of_markers_to_remove, body_index,scene_state);

  removeIndices(indices_of_markers_to_remove, [&](MarkerIndex i){
    visitor.visitMarker(i);
    scene_state.removeMarker(i);
  });
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
static void
removeBodyFromSceneState(
  BodyIndex body_index,
  SceneState &scene_state,
  const Visitor &visitor
)
{
  vector<BodyIndex> body_indices_to_remove;
  postOrderTraverseBodyBranch(body_index, scene_state, body_indices_to_remove);

  removeIndices(body_indices_to_remove, [&](BodyIndex i){
    removeMarkersOnBody(i, scene_state, visitor);
    visitor.visitBody(i);
    scene_state.removeBody(i);
  });
}


#endif /* SCENESTATE_HPP_ */
