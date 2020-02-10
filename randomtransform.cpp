#include "randomtransform.hpp"

#include "randomfloat.hpp"
#include "randompoint.hpp"
#include "coordinateaxes.hpp"
#include "eigenconv.hpp"
#include "point.hpp"
#include "maketransform.hpp"
#include "randomcoordinateaxes.hpp"


Transform randomTransform(RandomEngine &engine)
{
  CoordinateAxes coordinate_axes = randomCoordinateAxes(engine);
  Point origin = randomPoint(engine);
  return makeTransform(coordinate_axes,origin);
}
