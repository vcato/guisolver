#include "scenehandles.hpp"
#include "scenestate.hpp"
#include "scene.hpp"

extern SceneHandles createSceneObjects(const SceneState &state, Scene &scene);

extern void
  updateSceneStateFromSceneObjects(
    SceneState &,
    const Scene &,
    const SceneHandles &
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
  createDistanceErrorInScene(
    Scene &scene,
    SceneHandles &,
    const SceneState::DistanceError &
  );

extern void
  createMarkerInScene(
    Scene &scene,
    SceneHandles &scene_handles,
    const SceneState &,
    MarkerIndex
  );
