#include "scenestate.hpp"
#include "rotationvector.hpp"


inline SceneState::XYZ xyzState(const Point &p)
{
  return {p.x(), p.y(), p.z()};
}


inline SceneState::XYZ xyzState(const Vec3 &v)
{
  return {v.x, v.y, v.z};
}


inline TransformState transformState(const Transform &t)
{
  SceneState::XYZ rotation_state = xyzState(rotationVectorDeg(t.rotation()));
  SceneState::XYZ translation_state = xyzState(t.translation());
  return {translation_state, rotation_state};
}


inline Vec3 vec3(const SceneState::XYZ &arg)
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


inline TranslationState translationStateOf(const TransformState &arg)
{
  return arg.translation;
}


inline RotationState rotationStateOf(const TransformState &arg)
{
  return arg.rotation;
}


inline Vec3 translationValues(const TransformState &t)
{
  return vec3(t.translation);
}


inline Vec3 rotationValuesDeg(const RotationState &arg)
{
  return vec3(arg);
}


inline Vec3 rotationValuesDeg(const TransformState &transform_state)
{
  return vec3(transform_state.rotation);
}


inline Vec3 rotationValuesRad(const TransformState &t)
{
  float x = t.rotation.x*M_PI/180;
  float y = t.rotation.y*M_PI/180;
  float z = t.rotation.z*M_PI/180;
  return {x,y,z};
}


inline void
  setTranslationValues(TransformState &result, const Vec3 &translation)
{
  result.translation = xyzState(translation);
}


inline void setRotationValuesDeg(TransformState &result,const Vec3 &rotation)
{
  result.rotation = xyzState(rotation);
}
