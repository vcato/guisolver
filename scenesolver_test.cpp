#include "scenesolver.hpp"

#include <random>
#include <iostream>
#include "coordinateaxes.hpp"
#include "vec3.hpp"
#include "maketransform.hpp"
#include "sceneerror.hpp"
#include "eigenconv.hpp"
#include "rotationvector.hpp"
#include "transformstate.hpp"

using std::cerr;


using Quaternion = Eigen::Quaternion<float>;
using RandomEngine = std::mt19937;


static float randomFloat(float begin, float end, RandomEngine &engine)
{
  return std::uniform_real_distribution<float>(begin,end)(engine);
}


static Point randomPoint(RandomEngine &engine)
{
  float x = randomFloat(-1,1,engine);
  float y = randomFloat(-1,1,engine);
  float z = randomFloat(-1,1,engine);
  return {x,y,z};
}


static Quaternion randomUnitQuaternion(RandomEngine &engine)
{
  using Scalar = float;

  const Scalar u1 = randomFloat(0,1,engine),
               u2 = randomFloat(0, 2*EIGEN_PI,engine),
               u3 = randomFloat(0, 2*EIGEN_PI,engine);
  const Scalar a = sqrt(1 - u1),
               b = sqrt(u1);
  return Quaternion (a * sin(u2), a * cos(u2), b * sin(u3), b * cos(u3));
}


static CoordinateAxes randomCoordinateAxes(RandomEngine &engine)
{
  Quaternion q = randomUnitQuaternion(engine);
  return coordinateAxes(q.toRotationMatrix());
}


static Transform randomTransform(RandomEngine &engine)
{
  CoordinateAxes coordinate_axes = randomCoordinateAxes(engine);
  Point origin = randomPoint(engine);
  return makeTransform(coordinate_axes,origin);
}


static Transform inv(const Transform &t)
{
  return t.inverse();
}


static Point localizePoint(const Point &global,const Transform &transform)
{
  return inv(transform)*global;
}


static MarkerIndex
  addLocalMarkerTo(
    SceneState &result,
    const Point &local
  )
{
  MarkerIndex marker_index = result.addUnnamedMarker();
  result.marker(marker_index).position = local;
  result.marker(marker_index).is_local = true;
  return marker_index;
}


static MarkerIndex
  addGlobalMarkerTo(
    SceneState &result,
    const Point &local
  )
{
  MarkerIndex marker_index = result.addUnnamedMarker();
  result.marker(marker_index).position = local;
  result.marker(marker_index).is_local = false;
  return marker_index;
}


static void
  addDistanceErrorTo(
    SceneState &result,
    const Point &local,
    const Point &global
  )
{
  MarkerIndex local_marker_index = addLocalMarkerTo(result, local);
  MarkerIndex global_marker_index = addGlobalMarkerTo(result, global);
  SceneState::DistanceError &new_distance_error = result.addDistanceError();
  new_distance_error.optional_start_marker_index = local_marker_index;
  new_distance_error.optional_end_marker_index = global_marker_index;
}


namespace {
struct Example {
  SceneState scene_state;

  void addDistanceError(const Point &local, const Point &global)
  {
    addDistanceErrorTo(scene_state, local, global);
  }
};
}


static Example example(RandomEngine &engine)
{
  Example result;
  SceneState &scene_state = result.scene_state;

  // We should be able to create a random box transform and three
  // random global points, then find the equvalent local points
  // and setup the scene from that.
  Transform true_box_global = randomTransform(engine);
  Point global1 = randomPoint(engine);
  Point global2 = randomPoint(engine);
  Point global3 = randomPoint(engine);
  Point local1 = localizePoint(global1,true_box_global);
  Point local2 = localizePoint(global2,true_box_global);
  Point local3 = localizePoint(global3,true_box_global);
  result.addDistanceError(local1,global1);
  result.addDistanceError(local2,global2);
  result.addDistanceError(local3,global3);

  // Then we can set the box global transform to some other random
  // transform.

  scene_state.box.global = transformState(randomTransform(engine));
  return result;
}


int main()
{
  RandomEngine engine(/*seed*/1);
  SceneState scene_state = example(engine).scene_state;
  solveBoxPosition(scene_state);
  updateErrorsInState(scene_state);
  assert(sceneError(scene_state) < 0.002);
}
