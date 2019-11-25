#include "scenestate.hpp"
#include "expected.hpp"


extern void printSceneStateOn(std::ostream &, const SceneState &);
extern Expected<SceneState> scanSceneStateFrom(std::istream &);
