#include "transform3util.hpp"

#include "eigenconv.hpp"


static Transform inv(const Transform &t)
{
  return t.inverse();
}


static Point localizePoint(const Point &global,const Transform &transform)
{
  return inv(transform)*global;
}


Point3 localizePoint3(const Point3 &global,const Transform3 &transform)
{
  return
    makePoint3FromPoint(
      localizePoint(
        makePointFromPoint3(global),
        makeTransformFromTransform3(transform)
      )
    );
}

