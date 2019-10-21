inline Vec3 rotationVector(const Eigen::Matrix3f &r)
{
  Eigen::AngleAxisf a;
  a = r;
  Eigen::Vector3f axis = a.axis();
  float angle = a.angle();
  float x = axis.x() * angle;
  float y = axis.y() * angle;
  float z = axis.z() * angle;
  return {x,y,z};
}


inline Eigen::Matrix3f makeRotation(const Vec3 &rotation_vector)
{
  Eigen::Vector3f v = eigenVector3f(rotation_vector);
  float angle = v.norm();

  if (angle == 0) {
    return Eigen::Matrix3f::Identity();
  }

  Eigen::Vector3f axis = v.normalized();
  return Eigen::AngleAxisf(angle,axis).toRotationMatrix();
}
