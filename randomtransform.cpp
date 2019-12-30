#include "randomtransform.hpp"

#include "randomfloat.hpp"
#include "randompoint.hpp"
#include "coordinateaxes.hpp"
#include "eigenconv.hpp"
#include "point.hpp"
#include "maketransform.hpp"

using Quaternion = Eigen::Quaternion<float>;


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


Transform randomTransform(RandomEngine &engine)
{
  CoordinateAxes coordinate_axes = randomCoordinateAxes(engine);
  Point origin = randomPoint(engine);
  return makeTransform(coordinate_axes,origin);
}
