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

    DistanceError(
      MarkerIndex start_marker_index_arg,
      MarkerIndex end_marker_index_arg
    )
    : start_marker_index(start_marker_index_arg),
      end_marker_index(end_marker_index_arg)
    {
    }

    float distance = 0;
    float desired_distance = 0;
    float weight = 1;
    float error = 0;
  };

  Transform box_global;
  Markers markers;
  DistanceErrors distance_errors;
  float total_error = 0;

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
    distance_errors.push_back({start_marker_index,end_marker_index});
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
};


#endif /* SCENESTATE_HPP_ */
