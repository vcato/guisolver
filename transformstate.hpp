#ifndef TRANSFORMSTATE_HPP_
#define TRANSFORMSTATE_HPP_


#include "scenestate.hpp"
#include "transform.hpp"
#include "point.hpp"


inline SceneState::XYZ xyzState(const Point &p)
{
  return {p.x(), p.y(), p.z()};
}


inline SceneState::XYZ xyzStateFromVec3(const Vec3 &v)
{
  return {v.x, v.y, v.z};
}


extern TransformState transformState(const Transform &t, float scale);


inline Point point(const SceneState::XYZ &arg)
{
  return {arg.x, arg.y, arg.z};
}


extern Transform makeScaledTransformFromState(const SceneState::Transform &arg);


extern Transform
  makeUnscaledTransformFromState(
    const SceneState::Transform &arg,
    float parent_global_scale
  );


extern Vec3 translationValues(const TransformState &t);


extern Vec3 rotationValuesDeg(const TransformState &transform_state);


#endif /* TRANSFORMSTATE_HPP_ */
