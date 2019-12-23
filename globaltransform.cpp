#include "globaltransform.hpp"

#include "maketransform.hpp"


Transform
  globalTransform(const Scene &scene, Scene::TransformHandle transform_id)
{
  Point translation = scene.translation(transform_id);
  CoordinateAxes coordinate_axes = scene.coordinateAxes(transform_id);
  return makeTransform(coordinate_axes, translation);
}
