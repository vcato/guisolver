#include "sceneerror.hpp"

#include "scenestate.hpp"


using Vector3f = Eigen::Vector3f;


namespace {


struct RotationValues {
  float x,y,z;
};


}


static float dot(Vector3f a,Vector3f b)
{
  return a.dot(b);
}


float sceneError(const SceneState &scene_state)
{
  using Scalar = float;
  Scalar error = 0;

  // Add an error for each line
  for (auto &line : scene_state.lines) {
    Point start_predicted = scene_state.box_global * line.start;
    Point end_predicted = line.end;
    Point delta = start_predicted - end_predicted;
    error += dot(delta,delta);
  }

  return error;
}
