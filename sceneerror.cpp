#include "sceneerror.hpp"

#include "scenestate.hpp"
#include "sequence.hpp"
#include "indicesof.hpp"

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


float distanceError(const Point &start_predicted,const Point &end_predicted)
{
  Point delta = start_predicted - end_predicted;
  return dot(delta,delta);
}


float sceneError(const SceneState &scene_state)
{
  using Scalar = float;
  Scalar error = 0;

  for (auto i : indicesOf(scene_state.distance_errors)) {
    Point start_predicted = scene_state.distanceErrorStartPredicted(i);
    Point end_predicted = scene_state.distanceErrorEndPredicted(i);
    error += distanceError(start_predicted, end_predicted);
  }

  return error;
}
