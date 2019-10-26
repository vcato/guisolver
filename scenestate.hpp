#ifndef SCENESTATE_HPP_
#define SCENESTATE_HPP_

#include "vector.hpp"
#include "point.hpp"
#include "transform.hpp"
#include "markerindex.hpp"
#include "vector.hpp"


struct SceneState {
  using Points = vector<Point>;
  struct Marker;
  using Markers = vector<Marker>;

  struct Marker {
    Point position;
    bool is_local;
  };

  struct DistanceError {
    MarkerIndex start_marker_index;
    MarkerIndex end_marker_index;
  };

  Markers markers;
  using DistanceErrors = vector<DistanceError>;
  DistanceErrors distance_errors;
  Transform box_global;

  static Marker makeLocalMarker(const Point &position)
  {
    return Marker{position,/*is_local*/true};
  }

  static Marker makeGlobalMarker(const Point &position)
  {
    return Marker{position,/*is_local*/false};
  }

  MarkerIndex addMarker(const Marker &new_marker)
  {
    MarkerIndex new_index = markers.size();
    markers.push_back(new_marker);
    return new_index;
  }

  MarkerIndex addLocalMarker(const Point &position)
  {
    return addMarker(makeLocalMarker(position));
  }

  MarkerIndex addGlobalMarker(const Point &position)
  {
    return addMarker(makeGlobalMarker(position));
  }

  void
    addDistanceError(
      MarkerIndex start_marker_index,
      MarkerIndex end_marker_index
    )
  {
    distance_errors.push_back(
      DistanceError{start_marker_index,end_marker_index}
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
