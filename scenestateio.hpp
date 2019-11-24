#include "scenestate.hpp"
#include "expected.hpp"

#define ADD_SCAN_SCENE_STATE 0

extern void printSceneStateOn(std::ostream &, const SceneState &);

#if ADD_SCAN_SCENE_STATE
extern Expected<SceneState> scanSceneStateFrom(std::istream &);
#endif
