#include "eigenconv.hpp"
#include "scene.hpp"
#include "point.hpp"


inline Point
  makePointFromScenePoint(const Scene::Point &p)
{
  return eigenVector3f(p);
}


inline Scene::Point makeScenePointFromPoint(const Point &p)
{
  return vec3(p);
}
