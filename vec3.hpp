#ifndef VEC3_HPP_
#define VEC3_HPP_

#include <iostream>


struct Vec3 {
  using Scalar = float;
  Scalar x,y,z;

  Vec3(Scalar x_arg,Scalar y_arg,Scalar z_arg)
  : x(x_arg), y(y_arg), z(z_arg)
  {
  }

  bool operator==(const Vec3 &arg) const
  {
    return x==arg.x && y==arg.y && z==arg.z;
  }

  bool operator!=(const Vec3 &arg) const
  {
    return !operator==(arg);
  }

  Vec3& operator+=(const Vec3 &arg)
  {
    x += arg.x;
    y += arg.y;
    z += arg.z;
    return *this;
  }

  friend Vec3 operator*(Vec3 a, Scalar b)
  {
    a.x *= b;
    a.y *= b;
    a.z *= b;
    return a;
  }

  friend Vec3 operator/(Vec3 a, Scalar b)
  {
    a.x /= b;
    a.y /= b;
    a.z /= b;
    return a;
  }
};


inline Vec3 operator-(Vec3 a,Vec3 b)
{
  auto x = a.x - b.x;
  auto y = a.y - b.y;
  auto z = a.z - b.z;
  return {x,y,z};
}


inline std::ostream &operator<<(std::ostream &stream, const Vec3 &arg)
{
  stream << "Vec3(" << arg.x << "," << arg.y << "," << arg.z << ")";
  return stream;
}


#endif /* VEC3_HPP_ */
