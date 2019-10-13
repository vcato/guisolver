#ifndef SCENE_HPP_
#define SCENE_HPP_

#include <cstdlib>
#include <functional>
#include "point.hpp"
#include "coordinateaxes.hpp"


struct Scene {
  using Point = ::Point;
  using Vector = Vec3;

  struct TransformHandle {
    const size_t index;
  };

  struct LineHandle : TransformHandle {
    LineHandle(size_t index) : TransformHandle{index} {}
  };

  virtual TransformHandle top() const = 0;
  virtual TransformHandle createSphere(TransformHandle parent) = 0;
  virtual TransformHandle createBox(TransformHandle parent) = 0;
  virtual LineHandle createLine(TransformHandle parent) = 0;

  TransformHandle createSphere() { return createSphere(top()); }
  TransformHandle createBox() { return createBox(top()); }

  virtual void setGeometryScale(TransformHandle,float x,float y,float z) = 0;
  virtual void setCoordinateAxes(TransformHandle,Vector x,Vector y,Vector z) = 0;
  virtual CoordinateAxes coordinateAxes(TransformHandle) const = 0;
  virtual void setTranslation(TransformHandle,Point) = 0;
  virtual Point translation(TransformHandle) const = 0;
  virtual void setColor(TransformHandle,float r,float g,float b) = 0;
  virtual void setStartPoint(LineHandle,Point) = 0;
  virtual void setEndPoint(LineHandle,Point) = 0;
  virtual Point worldPoint(Point local,TransformHandle) const = 0;

  std::function<void()> changing_callback;
  std::function<void()> changed_callback;
};

#endif /* SCENE_HPP_ */
