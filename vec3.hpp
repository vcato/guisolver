#ifndef VEC3_HPP_
#define VEC3_HPP_


struct Vec3 {
  float x,y,z;

  Vec3(float x_arg,float y_arg,float z_arg)
  : x(x_arg), y(y_arg), z(z_arg)
  {
  }
};


inline Vec3 operator*(Vec3 a,float b)
{
  a.x *= b;
  a.y *= b;
  a.z *= b;
  return a;
}


#endif /* VEC3_HPP_ */
