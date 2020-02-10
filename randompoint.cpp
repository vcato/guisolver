#include "randompoint.hpp"

#include "randomfloat.hpp"
#include "randomvec3.hpp"
#include "vec3.hpp"


static Point makePointFromVec3(const Vec3 &v)
{
  return {v.x, v.y, v.z};
}


Point randomPoint(RandomEngine &engine)
{
  return makePointFromVec3(randomVec3(engine));
}
