#include "randomtransform3.hpp"

#include "randomcoordinateaxes.hpp"
#include "randompoint3.hpp"


Transform3 randomTransform3(RandomEngine &engine)
{
  CoordinateAxes coordinate_axes = randomCoordinateAxes(engine);
  Point3 origin = randomPoint3(engine);
  return {coordinate_axes, origin};
}
