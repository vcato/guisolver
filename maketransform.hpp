#include "transform.hpp"
#include "coordinateaxes.hpp"
#include "point.hpp"


extern Transform makeTransform(const CoordinateAxes &axes,const Point &origin);
extern void setTransformTranslation(Transform &box_global, const Vec3 &v);
extern void setTransformRotation(Transform &box_global, const Vec3 &v);
