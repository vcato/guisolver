#include "sceneerror.hpp"

#include "scenestate.hpp"
#include "sequence.hpp"
#include "indicesof.hpp"
#include "globaltransform.hpp"

using std::cerr;
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


static float distanceBetween(const Point &a,const Point &b)
{
  return sqrt(squaredDistanceBetween(a,b));
}


static void
  updateDistanceErrorInState(
    SceneState::DistanceError &distance_error,
    SceneState &scene_state
  )
{
  bool have_both_markers = distance_error.hasStart() && distance_error.hasEnd();

  if (!have_both_markers) {
    distance_error.maybe_distance = {};
    distance_error.error = 0;
    assert(!distance_error.maybe_distance);
    return;
  }

  MarkerIndex start_marker_index = distance_error.optional_start->marker.index;
  MarkerIndex end_marker_index = distance_error.optional_end->marker.index;
  Point start_predicted = markerPredicted(scene_state, start_marker_index);
  Point end_predicted = markerPredicted(scene_state, end_marker_index);
  float distance = distanceBetween(start_predicted, end_predicted);
  float desired_distance = distance_error.desired_distance;
  float weight = distance_error.weight;
  distance_error.maybe_distance = distance;
  distance_error.error = squared(distance - desired_distance) * weight;
}


void updateErrorsInState(SceneState &scene_state)
{
  float total_error = 0;

  for (auto i : indicesOf(scene_state.distance_errors)) {
    SceneState::DistanceError &distance_error = scene_state.distance_errors[i];
    updateDistanceErrorInState(distance_error, scene_state);
    total_error += distance_error.error;
  }

  scene_state.total_error = total_error;
}


float sceneError(const SceneState &scene_state)
{
  return scene_state.total_error;
}
