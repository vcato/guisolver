#include "scenestate.hpp"
#include "scenehandles.hpp"
#include "changesceneerror.hpp"

#if !CHANGE_SCENE_ERROR
extern void solveBoxPosition(SceneState &);
#else
extern void solveBoxPosition(SceneState &, const SceneSetup &);
#endif
