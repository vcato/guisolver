#ifndef VEC3_HPP_
#define VEC3_HPP_

#include <iostream>
#include <cmath>


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


inline Vec3
crossProduct(const Vec3 &a, const Vec3 &b)
{
  auto x = a.y*b.z - a.z*b.y;
  auto y = a.z*b.x - a.x*b.z;
  auto z = a.x*b.y - a.y*b.x;
  return {x,y,z};
}


inline Vec3::Scalar dot(const Vec3 &a, const Vec3 &b)
{
  return a.x*b.x + a.y*b.y + a.z*b.z;
}


inline Vec3::Scalar magnitude(const Vec3 &arg)
{
  using std::sqrt;
  return sqrt(dot(arg, arg));
}


inline Vec3 normalized(const Vec3 &arg)
{
  return arg/magnitude(arg);
}


inline std::ostream &operator<<(std::ostream &stream, const Vec3 &arg)
{
  stream << "Vec3(" << arg.x << "," << arg.y << "," << arg.z << ")";
  return stream;
}


#endif /* VEC3_HPP_ */
