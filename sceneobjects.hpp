#include "scenehandles.hpp"
#include "scenestate.hpp"
#include "scene.hpp"


extern SceneHandles createSceneObjects(const SceneState &state, Scene &scene);
extern void removeExistingManipulator(SceneHandles &, Scene &);

extern void
  destroySceneObjects(
    Scene &scene,
    const SceneState &state,
    const SceneHandles &
  );

extern void
  updateSceneStateFromSceneObjects(
    SceneState &,
    const Scene &,
    const SceneHandles &
  );

extern void
  updateBodyMeshPositionInScene(
    BodyIndex,
    MeshIndex,
    MeshPositionIndex,
    Scene &,
    const SceneHandles &,
    const SceneState &
  );

extern void
  updateSceneObjects(
    Scene &scene,
    const SceneHandles &scene_handles,
    const SceneState &state
  );

extern void
  removeDistanceErrorFromScene(
    Scene &scene,
    SceneHandles::DistanceErrors &distance_errors,
    int index
  );

extern void
  removeMarkerFromScene(
    Scene &scene,
    SceneHandles &,
    MarkerIndex index
  );

extern void
  removeVariableFromScene(
    Scene &scene,
    SceneHandles &,
    VariableIndex
  );

extern void
  removeMarkerObjectFromScene(
    MarkerIndex index,
    Scene &scene,
    SceneHandles &scene_handles
  );

extern void
  createDistanceErrorInScene(
    Scene &,
    SceneHandles &,
    const SceneState &,
    DistanceErrorIndex
  );

extern void
  createMarkerObjectInScene(
    MarkerIndex marker_index,
    Scene &scene,
    SceneHandles &scene_handles,
    const SceneState &state
  );

extern void
  createMarkerInScene(Scene &, SceneHandles &, const SceneState &, MarkerIndex);

extern void
  createBodyInScene(
    Scene &,
    SceneHandles &,
    const SceneState &,
    BodyIndex
  );

extern void
  createBoxInScene(
    Scene &scene,
    SceneHandles &scene_handles,
    BodyIndex body_index,
    BoxIndex i
  );

extern void
  createLineInScene(
    Scene &scene,
    SceneHandles &scene_handles,
    BodyIndex body_index,
    LineIndex line_index,
    const SceneState &
  );

extern void
  createMeshInScene(
    Scene &,
    SceneHandles &,
    BodyIndex parent_body_index,
    MeshIndex,
    const SceneState &
  );

extern void
  createBodyObjectsInScene(
    BodyIndex body_index,
    Scene &,
    SceneHandles &,
    const SceneState &
  );

extern void
  removeBodyFromScene(
    Scene &,
    SceneHandles &,
    const SceneState &,
    BodyIndex
  );

extern void
  removeBoxFromScene(
    Scene &,
    SceneHandles &,
    const SceneState &,
    BodyIndex,
    BoxIndex
  );

extern void
  removeLineFromScene(
    Scene &,
    SceneHandles &,
    const SceneState &,
    BodyIndex,
    LineIndex
  );

extern void
  removeMeshFromScene(
    Scene &,
    SceneHandles &,
    const SceneState &,
    BodyIndex,
    MeshIndex
  );

extern void
  createBodyBranchObjectsInScene(
    BodyIndex body_index,
    Scene &scene,
    SceneHandles &scene_handles,
    const SceneState &scene_state
  );

extern void
  removeBodyBranchObjectsFromScene(
    BodyIndex,
    Scene &,
    SceneHandles &,
    const SceneState &
  );

extern void
  updateBodyTranslateManipulator(
    Scene &,
    Scene::TransformHandle body_transform_handle,
    Scene::TransformHandle manipulator_handle
  );

extern void
  updateBodyRotateManipulator(
    Scene &,
    Scene::TransformHandle body_transform_handle,
    Scene::TransformHandle manipulator_handle
  );

extern void
  updateMarkerTranslateManipulator(
    Scene &,
    Scene::TransformHandle marker_transform_handle,
    Scene::TransformHandle manipulator_handle
  );

extern void
  updateBodyBoxScaleManipulator(
    Scene &,
    Scene::GeometryHandle box_geometry_handle,
    Scene::GeometryHandle manipulator
  );

extern void
  updateBodyBoxFromScaleManipulator(
    Scene::GeometryHandle box_handle,
    Scene::GeometryHandle manipulator,
    Scene &
  );

extern void
  updateBodyMeshScaleManipulator(
    Scene &,
    Scene::MeshHandle,
    Scene::GeometryHandle manipulator
  );

extern void
  updateBodyMeshFromScaleManipulator(
    Scene::MeshHandle mesh_handle,
    Scene::GeometryHandle manipulator,
    Scene &scene
  );

extern Vec3
  bodyMeshPositionRelativeToBody(
    BodyMeshPosition body_mesh_position,
    const SceneState &scene_state
  );

extern void
  updateBodyMeshPositionManipulator(
    Scene::TransformHandle manipulator,
    BodyMeshPosition body_mesh_position,
    const SceneState &scene_state,
    Scene &scene
  );

extern void
  updateBodyMeshPositionFromManipulator(
    Scene::TransformHandle manipulator,
    BodyMeshPosition body_mesh_position,
    SceneState &scene_state,
    Scene &scene,
    const SceneHandles &scene_handles
  );
