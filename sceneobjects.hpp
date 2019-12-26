#include "scenehandles.hpp"
#include "scenestate.hpp"
#include "scene.hpp"


extern SceneHandles createSceneObjects(const SceneState &state, Scene &scene);
extern void destroySceneObjects(Scene &scene, const SceneHandles &);

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
  removeMarkerFromScene(
    Scene &scene,
    SceneHandles::Markers &,
    MarkerIndex index
  );

extern void
  createDistanceErrorInScene(
    Scene &,
    SceneHandles &,
    const SceneState &,
    DistanceErrorIndex
  );

extern void
createMarkerInScene(Scene &, SceneHandles &, const SceneState &, MarkerIndex);

void
  createBodyInScene(
    Scene &,
    SceneHandles &,
    const SceneState &,
    BodyIndex
  );

void
  removeBodyFromScene(
    Scene &,
    SceneHandles &,
    const SceneState &,
    BodyIndex
  );
