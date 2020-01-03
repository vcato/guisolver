#ifndef SCENEHANDLES_HPP_
#define SCENEHANDLES_HPP_

#include "vector.hpp"
#include "scene.hpp"
#include "markerindex.hpp"
#include "bodyindex.hpp"
#include "matchconst.hpp"


struct SceneHandles {
  struct Marker;
  struct Body;
  struct DistanceError;
  using TransformHandle = Scene::TransformHandle;
  using GeometryHandle = Scene::GeometryHandle;
  using SphereAndTransformHandle = Scene::SphereAndTransformHandle;
  using BoxAndTransformHandle = Scene::BoxAndTransformHandle;
  using Markers = vector<Optional<Marker>>;
  using DistanceErrors = vector<DistanceError>;

  struct Box {
    GeometryHandle handle;

    Box(GeometryHandle handle)
    : handle(handle)
    {
    }
  };

  struct Body {
    TransformHandle transform_handle;
    vector<Box> boxes;

    Body(TransformHandle transform_handle, GeometryHandle box_handle)
    : transform_handle(transform_handle)
    {
      boxes.push_back(box_handle);
    }

    TransformHandle transformHandle() const { return transform_handle; }
  };

  struct Marker {
    SphereAndTransformHandle handle;
  };

  struct DistanceError {
    Scene::LineAndTransformHandle line;
  };

  vector<Optional<Body>> bodies;
  Markers markers;
  DistanceErrors distance_errors;

  template <typename SceneHandles>
  static MatchConst_t<Body, SceneHandles> &
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

  Body &body(BodyIndex i) { return body(i, *this); }
  const Body &body(BodyIndex i) const { return body(i, *this); }
  Marker &marker(MarkerIndex i) { return marker(i, *this); }
  const Marker &marker(MarkerIndex i) const { return marker(i, *this); }
};


#endif /* SCENEHANDLES_HPP_ */

