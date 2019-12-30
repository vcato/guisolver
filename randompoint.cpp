#include "randompoint.hpp"

#include "randomfloat.hpp"


Point randomPoint(RandomEngine &engine)
{
  float x = randomFloat(-1,1,engine);
  float y = randomFloat(-1,1,engine);
  float z = randomFloat(-1,1,engine);
  return {x,y,z};
}
