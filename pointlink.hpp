#ifndef POINTLINK_HPP_
#define POINTLINK_HPP_

#include "sceneelements.hpp"

#define ADD_BODY_MESH_POSITION_TO_POINT_LINK 0


struct PointLink {
#if !ADD_BODY_MESH_POSITION_TO_POINT_LINK
  Marker marker;

  PointLink(Marker marker) : marker(marker) {}

  Marker *markerPtr() { return &marker; }
#else
  Optional<Marker> maybe_marker;
  Optional<BodyMeshPosition> maybe_body_mesh_position;

  PointLink(Marker marker) : maybe_marker(marker) {}

  PointLink(BodyMeshPosition body_mesh_position)
  : maybe_body_mesh_position(body_mesh_position)
  {
  }

  Marker *markerPtr()
  {
    if (maybe_marker) {
      return &*maybe_marker;
    }

    return nullptr;
  }
#endif
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


#if !ADD_BODY_MESH_POSITION_TO_POINT_LINK
inline Optional<MarkerIndex>
makeMarkerIndexFromPoint(Optional<PointLink> maybe_point)
{
  if (!maybe_point) {
    return {};
  }

  return maybe_point->marker.index;
}
#endif


inline Marker *markerPtrFromPointLink(Optional<PointLink> &maybe_point_link)
{
  if (!maybe_point_link) {
    return nullptr;
  }

  return maybe_point_link->markerPtr();
}


#endif /* POINTLINK_HPP_ */
