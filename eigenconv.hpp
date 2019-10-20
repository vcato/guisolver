#include "vec3.hpp"
#include "coordinateaxes.hpp"


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
