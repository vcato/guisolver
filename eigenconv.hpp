#ifndef EIGENCONV_HPP_
#define EIGENCONV_HPP_

#include <Eigen/Dense>
#include "point.hpp"
#include "vec3.hpp"
#include "point3.hpp"
#include "coordinateaxes.hpp"
#include "transform3.hpp"
#include "maketransform.hpp"


inline Vec3 vec3(const Eigen::Vector3f &v)
{
  float x = v.x();
  float y = v.y();
  float z = v.z();
  return {x,y,z};
}


inline Eigen::Vector3f eigenVector3f(const Vec3 &v)
{
  float x = v.x;
  float y = v.y;
  float z = v.z;
  return {x,y,z};
}


inline CoordinateAxes coordinateAxes(const Eigen::Matrix3f &rotation)
{
  Vec3 x = vec3(rotation.col(0));
  Vec3 y = vec3(rotation.col(1));
  Vec3 z = vec3(rotation.col(2));
  return {x,y,z};
}


inline Point3 makePoint3FromPoint(const Point &p)
{
  return { p.x(), p.y(), p.z() };
}


inline Point makePointFromPoint3(const Point3 &p)
{
  return { p.x, p.y, p.z };
}


inline Transform makeTransformFromTransform3(const Transform3 &transform)
{
  return makeTransform(transform.axes, makePointFromPoint3(transform.origin));
}


#endif /* EIGENCONV_HPP_ */
