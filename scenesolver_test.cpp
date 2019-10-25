#include "scenesolver.hpp"

#include <random>
#include <iostream>
#include "coordinateaxes.hpp"
#include "vec3.hpp"
#include "maketransform.hpp"
#include "sceneerror.hpp"
#include "eigenconv.hpp"


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


static void
  addLineTo(SceneState &result,const Point &local, const Point &global)
{
  MarkerIndex local_marker_index = result.addLocalMarker(local);
  MarkerIndex global_marker_index = result.addGlobalMarker(global);
  result.addLine(local_marker_index, global_marker_index);
}


static SceneState exampleSceneState(RandomEngine &engine)
{
  SceneState result;

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
  addLineTo(result,local1,global1);
  addLineTo(result,local2,global2);
  addLineTo(result,local3,global3);

  // Then we can set the box global transform to some other random
  // transform.

  result.box_global = randomTransform(engine);
  return result;
}


int main()
{
  RandomEngine engine(/*seed*/1);
  SceneState scene_state = exampleSceneState(engine);
  solveBoxPosition(scene_state);
  assert(sceneError(scene_state) < 0.002);
}
