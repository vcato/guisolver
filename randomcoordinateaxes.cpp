#include "randomcoordinateaxes.hpp"

#include <Eigen/Geometry>
#include "randomfloat.hpp"
#include "eigenconv.hpp"


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


CoordinateAxes randomCoordinateAxes(RandomEngine &engine)
{
  Quaternion q = randomUnitQuaternion(engine);
  return coordinateAxes(q.toRotationMatrix());
}



