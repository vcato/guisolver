#include "scenehandles.hpp"
#include "scenestate.hpp"


extern SceneHandles createSceneObjects(const SceneState &state, Scene &scene);

extern void
  updateBoxInScene(
    Scene &scene,
    const Scene::TransformHandle &box_handle,
    const SceneState::Box &
  );

extern void
  updateDistanceErrorsInScene(
    Scene &scene,
    const SceneHandles &scene_handles,
    const SceneState &scene_state
  );

