#ifndef POSITIONSTATE_HPP_
#define POSITIONSTATE_HPP_


#include "scenestate.hpp"
#include "point.hpp"


inline Point makePoint(const MarkerPosition &arg)
{
  return {arg.x, arg.y, arg.z};
}


inline MarkerPosition makeMarkerPosition(const Eigen::Vector3f &arg)
{
  return {arg.x(), arg.y(), arg.z()};
}


#endif /* POSITIONSTATE_HPP_ */
