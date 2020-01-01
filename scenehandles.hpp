#ifndef SCENEHANDLES_HPP_
#define SCENEHANDLES_HPP_

#include "vector.hpp"
#include "scene.hpp"
#include "markerindex.hpp"
#include "bodyindex.hpp"
#include "matchconst.hpp"


struct SceneHandles {
  struct Marker;
  struct DistanceError;
  using TransformHandle = Scene::TransformHandle;
  using Markers = vector<Optional<Marker>>;
  using DistanceErrors = vector<DistanceError>;

  struct Marker {
    TransformHandle handle;
  };

  struct DistanceError {
    Scene::LineHandle line;
  };

  template <typename SceneHandles>
  static MatchConst_t<TransformHandle, SceneHandles> &
  body(BodyIndex i, SceneHandles &scene_handles)
  {
    assert(scene_handles.bodies[i]);
    return *scene_handles.bodies[i];
  }

  template <typename SceneHandles>
  static MatchConst_t<Marker, SceneHandles> &
  marker(MarkerIndex i, SceneHandles &scene_handles)
  {
    assert(scene_handles.markers[i]);
    return *scene_handles.markers[i];
  }

  TransformHandle &body(BodyIndex i) { return body(i, *this); }
  const TransformHandle &body(BodyIndex i) const { return body(i, *this); }
  Marker &marker(MarkerIndex i) { return marker(i, *this); }
  const Marker &marker(MarkerIndex i) const { return marker(i, *this); }

  vector<Optional<TransformHandle>> bodies;
  Markers markers;
  DistanceErrors distance_errors;
};


#endif /* SCENEHANDLES_HPP_ */

