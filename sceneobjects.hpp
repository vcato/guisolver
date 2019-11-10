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

void
  removeDistanceErrorFromScene(
    Scene &scene,
    SceneHandles::DistanceErrors &distance_errors,
    int index
  );

extern void
  createDistanceErrorInScene(
    Scene &scene,
    vector<SceneHandles::DistanceError> &distance_error_handles
  );
