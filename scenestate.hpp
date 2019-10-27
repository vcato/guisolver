#ifndef SCENESTATE_HPP_
#define SCENESTATE_HPP_

#include "vector.hpp"
#include "point.hpp"
#include "transform.hpp"
#include "markerindex.hpp"
#include "vector.hpp"


struct SceneState {
  struct Marker;
  struct DistanceError;
  using Points = vector<Point>;
  using Markers = vector<Marker>;
  using DistanceErrors = vector<DistanceError>;
  using String = std::string;

  struct Marker {
    Point position;
    bool is_local;
    String name;
  };

  struct DistanceError {
    MarkerIndex start_marker_index;
    MarkerIndex end_marker_index;
    float distance;
    float desired_distance;
    float error;
  };

  Markers markers;
  DistanceErrors distance_errors;
  Transform box_global;

  MarkerIndex addMarker()
  {
    MarkerIndex new_index = markers.size();
    markers.emplace_back();
    return new_index;
  }

  void
    addDistanceError(
      MarkerIndex start_marker_index,
      MarkerIndex end_marker_index
    )
  {
    distance_errors.push_back(
      DistanceError{
        start_marker_index,
        end_marker_index,
        /*distance*/0,
        /*desired_distance*/0,
        /*error*/0
      }
    );
  }

  Point markerPredicted(int marker_index) const
  {
    if (markers[marker_index].is_local) {
      return box_global * markers[marker_index].position;
    }
    else {
      return markers[marker_index].position;
    }
  }

  Point distanceErrorStartPredicted(int line_index) const
  {
    return markerPredicted(distance_errors[line_index].start_marker_index);
  }

  Point distanceErrorEndPredicted(int line_index) const
  {
    return markerPredicted(distance_errors[line_index].end_marker_index);
  }
};


#endif /* SCENESTATE_HPP_ */
