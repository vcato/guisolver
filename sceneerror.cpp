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


static float
  squaredDistanceBetween(
    const Point &start_predicted,const Point &end_predicted
  )
{
  Point delta = start_predicted - end_predicted;
  return dot(delta,delta);
}


static float squared(float x)
{
  return x*x;
}


void updateDistanceErrorsInState(SceneState &scene_state)
{
  for (auto i : indicesOf(scene_state.distance_errors)) {
    Point start_predicted = scene_state.distanceErrorStartPredicted(i);
    Point end_predicted = scene_state.distanceErrorEndPredicted(i);

    float distance =
      sqrt(squaredDistanceBetween(start_predicted, end_predicted));

    float desired_distance = scene_state.distance_errors[i].desired_distance;
    scene_state.distance_errors[i].distance = distance;
    scene_state.distance_errors[i].error = squared(distance - desired_distance);
  }
}


float sceneError(const SceneState &scene_state)
{
  using Scalar = float;
  Scalar error = 0;

  for (auto i : indicesOf(scene_state.distance_errors)) {
    error += scene_state.distance_errors[i].error;
  }

  return error;
}
