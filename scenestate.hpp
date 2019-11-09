#ifndef SCENESTATE_HPP_
#define SCENESTATE_HPP_

#include "vector.hpp"
#include "point.hpp"
#include "transform.hpp"
#include "markerindex.hpp"
#include "vector.hpp"
#include "optional.hpp"


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

  struct Box {
    Transform global;
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
  Markers markers;
  DistanceErrors distance_errors;
  float total_error = 0;

  MarkerIndex addMarker()
  {
    MarkerIndex new_index = markers.size();
    markers.emplace_back();
    return new_index;
  }

  DistanceError& addDistanceError()
  {
    distance_errors.emplace_back();
    return distance_errors.back();
  }

  Point markerPredicted(int marker_index) const
  {
    if (markers[marker_index].is_local) {
      return box.global * markers[marker_index].position;
    }
    else {
      return markers[marker_index].position;
    }
  }
};


#endif /* SCENESTATE_HPP_ */
