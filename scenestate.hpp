#ifndef SCENESTATE_HPP_
#define SCENESTATE_HPP_

#include "vector.hpp"
#include "point.hpp"
#include "transform.hpp"
#include "markerindex.hpp"
#include "vector.hpp"
#include "optional.hpp"
#include "removeindexfrom.hpp"


class SceneState {
  public:
    struct Marker;
    struct DistanceError;
    using Points = vector<Point>;
    using Markers = vector<Marker>;
    using DistanceErrors = vector<DistanceError>;
    using String = std::string;

    struct Marker {
      using Name = String;
      Point position;
      bool is_local;
      Name name;
    };

    struct Box {
      Transform global = Transform::Identity();
      float scale_x =  5.0;
      float scale_y =  0.1;
      float scale_z = 10.0;
    };

    struct DistanceError {
      Optional<MarkerIndex> optional_start_marker_index;
      Optional<MarkerIndex> optional_end_marker_index;
      Optional<float> maybe_distance;
      float desired_distance = 0;
      float weight = 1;
      float error = 0;
    };

    Box box;
    DistanceErrors distance_errors;
    float total_error = 0;

    const Markers &markers() const { return _markers; }

    Marker &marker(MarkerIndex index) { return _markers[index]; }
    const Marker &marker(MarkerIndex index) const { return _markers[index]; }

    MarkerIndex addMarker(const String &name)
    {
      MarkerIndex new_index = _markers.size();
      _markers.emplace_back();
      _markers.back().name = name;
      return new_index;
    }

    MarkerIndex addUnnamedMarker()
    {
      return addMarker("");
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

    DistanceError& addDistanceError()
    {
      distance_errors.emplace_back();
      return distance_errors.back();
    }

    void removeDistanceError(int index)
    {
      removeIndexFrom(distance_errors, index);
    }

    Point markerPredicted(int marker_index) const;

  private:
    Markers _markers;

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


extern MarkerIndex createMarkerInState(SceneState &state, bool is_local);
extern vector<SceneState::Marker::Name> markerNames(const SceneState &state);

#endif /* SCENESTATE_HPP_ */
