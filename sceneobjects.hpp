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

extern SceneHandles::DistanceError createDistanceErrorInScene(Scene &scene);
