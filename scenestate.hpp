#ifndef SCENESTATE_HPP_
#define SCENESTATE_HPP_

#include "vector.hpp"
#include "markerindex.hpp"
#include "bodyindex.hpp"
#include "vector.hpp"
#include "optional.hpp"
#include "removeindexfrom.hpp"


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
      bool x = true;
      bool y = true;
      bool z = true;
    };

    struct TransformSolveFlags {
      XYZSolveFlags translation;
      XYZSolveFlags rotation;
    };

    struct Body {
      Transform transform;
      XYZ scale = { 5.0, 0.1, 10.0 };
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

    MarkerIndex createMarker(const String &name)
    {
      MarkerIndex new_index = _markers.size();
      _markers.emplace_back();
      _markers.back().name = name;
      return new_index;
    }

    BodyIndex createBody(Optional<BodyIndex> maybe_parent_index)
    {
      BodyIndex new_index = _bodies.size();
      _bodies.emplace_back();
      _bodies.back().maybe_parent_index = maybe_parent_index;
      return new_index;
    }

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

    void removeBody(BodyIndex index_to_remove)
    {
      assert(!bodyHasChildren(index_to_remove));
      removeIndexFrom(_bodies, index_to_remove);
    }

    DistanceError& createDistanceError()
    {
      distance_errors.emplace_back();
      return distance_errors.back();
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

extern MarkerIndex
  createMarkerInState(
    SceneState &state,
    Optional<BodyIndex> maybe_body_index
  );

extern vector<SceneState::Marker::Name> markerNames(const SceneState &state);

extern BodyIndex
  createBodyInState(SceneState &, Optional<BodyIndex> maybe_parent_index);

#endif /* SCENESTATE_HPP_ */
