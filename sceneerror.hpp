#define USE_SOLVER 0

#include "scenesetup.hpp"
#include "scenestate.hpp"
#include "vec3.hpp"

#if USE_SOLVER
extern void
  solveBoxPosition(
    const SceneSetup &scene_setup,
    const Transform &box_global_transform
  );
#endif


extern float sceneError(const SceneState &);
