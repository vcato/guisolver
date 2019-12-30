#include "randomengine.hpp"


inline float randomFloat(float begin, float end, RandomEngine &engine)
{
  return std::uniform_real_distribution<float>(begin,end)(engine);
}
