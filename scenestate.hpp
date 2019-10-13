#ifndef SCENESTATE_HPP_
#define SCENESTATE_HPP_

#include "point.hpp"
#include "transform.hpp"


struct SceneState {
  struct Line {
    Point start;
    Point end;
  };

  std::vector<Line> lines;
  Transform box_global;
};


#endif /* SCENESTATE_HPP_ */
