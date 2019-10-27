#ifndef SCENEDATA_HPP_
#define SCENEDATA_HPP_

#include "scenehandles.hpp"
#include "scenestate.hpp"

struct SceneData {
  SceneHandles handles;
  SceneDescription description;
  SceneState state;
};


#endif /* SCENEDATA_HPP_ */
