#ifndef POSITIONSTATE_HPP_
#define POSITIONSTATE_HPP_


#include "scenestate.hpp"
#include "point.hpp"


inline Point makePointFromPositionState(const PositionState &arg)
{
  return {arg.x, arg.y, arg.z};
}


inline PositionState makePositionStateFromPoint(const Eigen::Vector3f &arg)
{
  return {arg.x(), arg.y(), arg.z()};
}


#endif /* POSITIONSTATE_HPP_ */
