#ifndef POINTLINK_HPP_
#define POINTLINK_HPP_

#include "sceneelements.hpp"


struct PointLink {
  Marker marker;

  PointLink(Marker marker) : marker(marker) {}

  Marker *markerPtr() { return &marker; }
};


inline Optional<PointLink>
makePointLink(const Optional<Marker> &maybe_marker)
{
  if (!maybe_marker) {
    return {};
  }
  else {
    return PointLink{*maybe_marker};
  }
}


inline Optional<MarkerIndex>
makeMarkerIndexFromPoint(Optional<PointLink> maybe_point)
{
  if (!maybe_point) {
    return {};
  }

  return maybe_point->marker.index;
}


inline Marker *markerPtrFromPointLink(Optional<PointLink> &maybe_point_link)
{
  if (!maybe_point_link) {
    return nullptr;
  }

  return maybe_point_link->markerPtr();
}


#endif /* POINTLINK_HPP_ */