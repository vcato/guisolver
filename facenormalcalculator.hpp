#include "vec3.hpp"


struct FaceNormalCalculator {
  Vec3 total{0,0,0};

  void addTriangle(const Vec3 &v1, const Vec3 &v2, const Vec3 &v3)
  {
    total += crossProduct(v2-v1, v3-v2);
  }

  Vec3 result() const
  {
    return normalized(total);
  }
};
