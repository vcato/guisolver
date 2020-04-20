#include "transformstate.hpp"

#include "rotationvector.hpp"
#include "vec3state.hpp"


TransformState transformState(const Transform &t, float scale)
{
  SceneState::XYZ rotation_state =
    xyzStateFromVec3(rotationVectorDeg(t.rotation()));

  SceneState::XYZ translation_state = xyzState(t.translation());
  return {translation_state, rotation_state, scale};
}


Transform makeScaledTransformFromState(const SceneState::Transform &arg)
{
  Transform box_global = Transform::Identity();
  setTransformTranslation(box_global, vec3FromXYZState(arg.translation));
  setTransformRotationDeg(box_global, vec3FromXYZState(arg.rotation));
  scaleTransform(box_global, arg.scale);
  return box_global;
}


Transform
makeUnscaledTransformFromState(
  const SceneState::Transform &arg,
  float parent_global_scale
)
{
  Transform box_global = Transform::Identity();

  setTransformTranslation(
    box_global, vec3FromXYZState(arg.translation) * parent_global_scale
  );

  setTransformRotationDeg(box_global, vec3FromXYZState(arg.rotation));
  return box_global;
}


Vec3 translationValues(const TransformState &t)
{
  return vec3FromXYZState(t.translation);
}


Vec3 rotationValuesDeg(const TransformState &transform_state)
{
  return vec3FromXYZState(transform_state.rotation);
}
