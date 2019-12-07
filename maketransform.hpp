#include "transform.hpp"
#include "coordinateaxes.hpp"
#include "point.hpp"


extern Transform makeTransform(const CoordinateAxes &axes,const Point &origin);
extern void setTransformTranslation(Transform &box_global, const Vec3 &v);
extern void setTransformRotationRad(Transform &, const Vec3 &rotation);
extern void setTransformRotationDeg(Transform &, const Vec3 &rotation);
