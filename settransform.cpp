#include "settransform.hpp"

#include "eigenconv.hpp"
#include "scenetransform.hpp"


void
  setTransform(
    Scene::TransformHandle transform_id,
    const Transform &transform_value,
    Scene &scene
  )
{
  CoordinateAxes coordinate_axes =
    coordinateAxes(transform_value.rotation());

  scene.setCoordinateAxes(transform_id, coordinate_axes);

  scene.setTranslation(
    transform_id, makeScenePointFromPoint(transform_value.translation())
  );
}
