#include "scenestate.hpp"
#include "expected.hpp"


extern void printSceneStateOn(std::ostream &, const SceneState &);
extern Expected<SceneState> scanSceneStateFrom(std::istream &);
extern void saveScene(const SceneState &scene_state, const std::string &path);
extern void loadScene(SceneState &scene_state, const std::string &path);
