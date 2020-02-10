#ifndef POSITIONSTATEPOINT3_HPP_
#define POSITIONSTATEPOINT3_HPP_


#include "scenestate.hpp"
#include "point3.hpp"


inline Point3 makePoint3FromPositionState(const PositionState &arg)
{
  return { arg.x, arg.y, arg.z };
}


inline PositionState makePositionStateFromPoint3(const Point3 &arg)
{
  return { arg.x, arg.y, arg.z };
}


#endif /* POSITIONSTATEPOINT3_HPP_ */
