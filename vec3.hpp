#ifndef VEC3_HPP_
#define VEC3_HPP_

#include <iostream>


struct Vec3 {
  float x,y,z;

  Vec3(float x_arg,float y_arg,float z_arg)
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
};


inline Vec3 operator*(Vec3 a,float b)
{
  a.x *= b;
  a.y *= b;
  a.z *= b;
  return a;
}


inline std::ostream &operator<<(std::ostream &stream, const Vec3 &arg)
{
  stream << "Vec3(" << arg.x << "," << arg.y << "," << arg.z << ")";
  return stream;
}


#endif /* VEC3_HPP_ */
