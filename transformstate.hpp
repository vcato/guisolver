#ifndef TRANSFORMSTATE_HPP_
#define TRANSFORMSTATE_HPP_


#include "scenestate.hpp"
#include "rotationvector.hpp"
#include "transform.hpp"
#include "maketransform.hpp"
#include "vec3state.hpp"


inline SceneState::XYZ xyzState(const Point &p)
{
  return {p.x(), p.y(), p.z()};
}


inline SceneState::XYZ xyzStateFromVec3(const Vec3 &v)
{
  return {v.x, v.y, v.z};
}


inline TransformState transformState(const Transform &t)
{
  SceneState::XYZ rotation_state =
    xyzStateFromVec3(rotationVectorDeg(t.rotation()));

  SceneState::XYZ translation_state = xyzState(t.translation());
  return {translation_state, rotation_state};
}


inline Point point(const SceneState::XYZ &arg)
{
  return {arg.x, arg.y, arg.z};
}


inline Transform makeTransformFromState(const SceneState::Transform &arg)
{
  Transform box_global = Transform::Identity();
  setTransformTranslation(box_global, vec3(arg.translation));
  setTransformRotationDeg(box_global, vec3(arg.rotation));
  return box_global;
}


inline Vec3 translationValues(const TransformState &t)
{
  return vec3(t.translation);
}


inline Vec3 rotationValuesDeg(const TransformState &transform_state)
{
  return vec3(transform_state.rotation);
}


#endif /* TRANSFORMSTATE_HPP_ */
