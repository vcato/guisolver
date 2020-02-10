#ifndef TRANSFORM3_HPP_
#define TRANSFORM3_HPP_


#include "coordinateaxes.hpp"
#include "point3.hpp"


struct Transform3 {
  CoordinateAxes axes;
  Point3 origin;
};


#endif /* TRANSFORM3_HPP_ */
