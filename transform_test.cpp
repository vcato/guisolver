#include "maketransform.hpp"


int main()
{
  CoordinateAxes axes = {{1,0,0},{0,1,0},{0,0,1}};
  Point origin = {0,0,0};
  Transform t = makeTransform(axes,origin);
  assert(t.matrix() == Transform::Identity().matrix());
}
