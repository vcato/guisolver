#ifndef SCENE_HPP_
#define SCENE_HPP_

#include <cstdlib>
#include <functional>


struct Scene {
  struct TransformHandle {
    const size_t index;
  };

  struct LineHandle : TransformHandle {
    LineHandle(size_t index) : TransformHandle{index} {}
  };

  struct Point {
    float x=0, y=0, z=0;

    Point(float x_arg,float y_arg,float z_arg)
    : x(x_arg), y(y_arg), z(z_arg)
    {
    }
  };

  virtual TransformHandle top() const = 0;
  virtual TransformHandle createSphere(TransformHandle parent) = 0;
  virtual TransformHandle createBox(TransformHandle parent) = 0;
  virtual LineHandle createLine(TransformHandle parent) = 0;

  TransformHandle createSphere() { return createSphere(top()); }
  TransformHandle createBox() { return createBox(top()); }

  virtual void setScale(TransformHandle,float x,float y,float z) = 0;
  virtual void setTranslation(TransformHandle,Point) = 0;
  virtual void setColor(TransformHandle,float r,float g,float b) = 0;
  virtual void setStartPoint(LineHandle,Point) = 0;
  virtual void setEndPoint(LineHandle,Point) = 0;
  virtual Point worldPoint(Point local,TransformHandle) const = 0;

  std::function<void()> changing_callback;
  std::function<void()> changed_callback;
};

#endif /* SCENE_HPP_ */
