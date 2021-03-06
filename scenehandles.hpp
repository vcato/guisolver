#ifndef SCENEHANDLES_HPP_
#define SCENEHANDLES_HPP_

#include "vector.hpp"
#include "scene.hpp"
#include "markerindex.hpp"
#include "bodyindex.hpp"
#include "matchconst.hpp"
#include "sceneelements.hpp"


struct OptionalManipulatedElement {
  Optional<BodyIndex> maybe_body_index;
  Optional<MarkerIndex> maybe_marker_index;
  Optional<BodyBox> maybe_body_box;
  Optional<BodyMesh> maybe_body_mesh;
  Optional<BodyMeshPositions> maybe_body_mesh_positions;
  Optional<BodyMeshPosition> maybe_body_mesh_position;
};


struct SceneHandles {
  struct Marker;
  struct Body;
  struct DistanceError;
  using TransformHandle = Scene::TransformHandle;
  using GeometryHandle = Scene::GeometryHandle;
  using MeshHandle = Scene::MeshHandle;
  using LineHandle = Scene::LineHandle;
  using Markers = vector<Optional<Marker>>;
  using DistanceErrors = vector<DistanceError>;

  struct Box {
    GeometryHandle handle;
  };

  struct Line {
    LineHandle handle;
  };

  struct Mesh {
    MeshHandle handle;
  };

  struct Body {
    TransformHandle transform_handle;
    vector<Box> boxes;
    vector<Line> lines;
    vector<Mesh> meshes;

    Body(TransformHandle transform_handle)
    : transform_handle(transform_handle)
    {
    }

    void addBox(GeometryHandle box_handle)
    {
      boxes.push_back(Box{box_handle});
    }

    void addLine(LineHandle line_handle)
    {
      lines.push_back(Line{line_handle});
    }

    void addMesh(MeshHandle mesh_handle)
    {
      meshes.push_back(Mesh{mesh_handle});
    }

    TransformHandle transformHandle() const { return transform_handle; }
  };

  struct Marker {
    private:
    TransformHandle transform_handle;
    GeometryHandle sphere_handle;
    public:

    Marker(TransformHandle transform_handle, GeometryHandle sphere_handle)
    : transform_handle(transform_handle),
      sphere_handle(sphere_handle)
    {
    }

    TransformHandle transformHandle() const { return transform_handle; }
    GeometryHandle sphereHandle() const { return sphere_handle; }
  };

  struct DistanceError {
    Scene::TransformHandle transform_handle;
    Scene::LineHandle line_handle;
  };

  Optional<TransformHandle> maybe_translate_manipulator;
  Optional<TransformHandle> maybe_rotate_manipulator;
  Optional<GeometryHandle> maybe_scale_manipulator;
  Optional<vector<GeometryHandle>> maybe_points;
  OptionalManipulatedElement maybe_manipulated_element;

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

