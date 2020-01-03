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
  using SphereAndTransformHandle = Scene::SphereAndTransformHandle;
  using BoxAndTransformHandle = Scene::BoxAndTransformHandle;
  using Markers = vector<Optional<Marker>>;
  using DistanceErrors = vector<DistanceError>;

  struct Marker {
    SphereAndTransformHandle handle;
  };

  struct DistanceError {
    Scene::LineAndTransformHandle line;
  };

  vector<Optional<BoxAndTransformHandle>> bodies;
  Markers markers;
  DistanceErrors distance_errors;

  template <typename SceneHandles>
  static MatchConst_t<BoxAndTransformHandle, SceneHandles> &
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

  BoxAndTransformHandle &body(BodyIndex i) { return body(i, *this); }
  const BoxAndTransformHandle &body(BodyIndex i) const { return body(i, *this); }
  Marker &marker(MarkerIndex i) { return marker(i, *this); }
  const Marker &marker(MarkerIndex i) const { return marker(i, *this); }
};


#endif /* SCENEHANDLES_HPP_ */

