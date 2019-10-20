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
  int n_lines = scene_state.lines.size();

  // Add an error for each line
  for (int i=0; i!=n_lines; ++i) {
    Point start_predicted =
      scene_state.box_global * scene_state.lineStartLocal(i);

    Point end_predicted = scene_state.lineEndGlobal(i);
    Point delta = start_predicted - end_predicted;
    error += dot(delta,delta);
  }

  return error;
}
