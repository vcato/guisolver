#include "randompoint3.hpp"

#include "randomfloat.hpp"
#include "randomvec3.hpp"


static Point3 makePoint3FromVec3(const Vec3 &v)
{
  return {v.x, v.y, v.z};
}


Point3 randomPoint3(RandomEngine &engine)
{
  return makePoint3FromVec3(randomVec3(engine));
}
