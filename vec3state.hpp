#include "vec3.hpp"
#include "scenestate.hpp"


inline Vec3 vec3FromXYZState(const SceneState::XYZ &arg)
{
  return {arg.x, arg.y, arg.z};
}


inline Vec3 rotationValuesDeg(const RotationState &arg)
{
  return vec3FromXYZState(arg);
}


inline PositionState makePositionStateFromVec3(const Vec3 &arg)
{
  return {arg.x, arg.y, arg.z};
}
