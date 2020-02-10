#include "randomvec3.hpp"

#include "randomfloat.hpp"


Vec3 randomVec3(RandomEngine &engine)
{
  float x = randomFloat(-1,1,engine);
  float y = randomFloat(-1,1,engine);
  float z = randomFloat(-1,1,engine);
  return {x,y,z};
}
