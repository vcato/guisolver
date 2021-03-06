#include "maketransform.hpp"

#include "eigenconv.hpp"
#include "rotationvector.hpp"


Transform makeTransform(const CoordinateAxes &axes,const Point &origin)
{
  Transform transform = Transform::Identity();
  transform(0,0) = axes.x.x;
  transform(1,0) = axes.x.y;
  transform(2,0) = axes.x.z;
  transform(0,1) = axes.y.x;
  transform(1,1) = axes.y.y;
  transform(2,1) = axes.y.z;
  transform(0,2) = axes.z.x;
  transform(1,2) = axes.z.y;
  transform(2,2) = axes.z.z;
  transform(0,3) = origin.x();
  transform(1,3) = origin.y();
  transform(2,3) = origin.z();
  return transform;
}


Vec3 transformTranslation(const Transform &transform)
{
  return vec3(transform.translation());
}


void
  setTransformTranslation(Transform &box_global, const Vec3 &v)
{
  box_global.translation() = eigenVector3f(v);
}


void setTransformRotationRad(Transform &result, const Vec3 &v)
{
  result.matrix().topLeftCorner<3,3>() = makeRotation(v);
}


void setTransformRotationDeg(Transform &result, const Vec3 &rotation)
{
  setTransformRotationRad(result, rotation*(M_PI/180));
}


void scaleTransform(Transform &result, float scale)
{
  result.matrix().topLeftCorner<3,3>() *= scale;
}
