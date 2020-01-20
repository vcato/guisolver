inline Vec3 vec3(const SceneState::XYZ &arg)
{
  return {arg.x, arg.y, arg.z};
}


inline Vec3 rotationValuesDeg(const RotationState &arg)
{
  return vec3(arg);
}
