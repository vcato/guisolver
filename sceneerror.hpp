#include "scenestate.hpp"
#include "scenehandles.hpp"
#include "changesceneerror.hpp"

#if !CHANGE_SCENE_ERROR
extern float sceneError(const SceneState &);
#else
extern float sceneError(const SceneState &, const SceneSetup &);
#endif
extern float distanceError(const Point &start,const Point &end);
