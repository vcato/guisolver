#ifndef SCENESTATE_HPP_
#define SCENESTATE_HPP_

#include <vector>
#include "point.hpp"
#include "transform.hpp"


struct SceneState {
  using Points = std::vector<Point>;

  struct Line {
    int start_local_index;
    int end_global_index;
  };

  Points locals;
  Points globals;
  std::vector<Line> lines;
  Transform box_global;

  void addLine(const Point &start_local,const Point &end_global)
  {
    int local_index = locals.size();
    locals.push_back(start_local);
    int global_index = globals.size();
    globals.push_back(end_global);
    lines.push_back(Line{local_index,global_index});
  }

  Point lineStartLocal(int line_index) const
  {
    return locals[lines[line_index].start_local_index];
  }

  Point lineEndGlobal(int line_index) const
  {
    return globals[lines[line_index].end_global_index];
  }
};


#endif /* SCENESTATE_HPP_ */
