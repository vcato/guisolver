#ifndef POINTLINK_HPP_
#define POINTLINK_HPP_

#include "sceneelements.hpp"


struct PointLink {
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
};


inline Optional<PointLink>
maybePointLink(const Optional<Marker> &maybe_marker)
{
  if (!maybe_marker) {
    return {};
  }
  else {
    return PointLink{*maybe_marker};
  }
}


inline Optional<PointLink>
maybePointLink(const Optional<BodyMeshPosition> &arg)
{
  if (arg) {
    return PointLink(*arg);
  }
  else {
    return {};
  }
}


inline Marker *markerPtrFromPointLink(Optional<PointLink> &maybe_point_link)
{
  if (!maybe_point_link) {
    return nullptr;
  }

  return maybe_point_link->markerPtr();
}


#endif /* POINTLINK_HPP_ */
