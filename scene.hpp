#ifndef SCENE_HPP_
#define SCENE_HPP_

#include <cstdlib>
#include <functional>
#include "point.hpp"
#include "coordinateaxes.hpp"
#include "optional.hpp"


struct Scene {
  using Point = ::Point;
  using Vector = Vec3;

  enum class DraggerType {
    translate,
    rotate,
    scale
  };

  struct TransformHandle {
    using Index = size_t;
    Index index;

    friend std::ostream&
      operator<<(std::ostream &stream, const TransformHandle &transform_handle)
    {
      stream << "TransformHandle(index=" << transform_handle.index << ")";
      return stream;
    }

    bool operator==(const TransformHandle &arg) const
    {
      return index == arg.index;
    }
  };

  struct LineHandle : TransformHandle {
    LineHandle(size_t index) : TransformHandle{index} {}
  };

  std::function<void()> changing_callback;
  std::function<void()> changed_callback;
  std::function<void()> selection_changed_callback;

  virtual TransformHandle top() const = 0;
  virtual TransformHandle createSphere(TransformHandle parent) = 0;
  virtual TransformHandle createBox(TransformHandle parent) = 0;
  virtual LineHandle createLine(TransformHandle parent) = 0;
  virtual void destroyLine(LineHandle) = 0;
  virtual void destroyObject(TransformHandle) = 0;
  virtual void setGeometryScale(TransformHandle,float x,float y,float z) = 0;
  virtual Vec3 geometryScale(TransformHandle) const = 0;
  virtual void setCoordinateAxes(TransformHandle,const CoordinateAxes &) = 0;
  virtual CoordinateAxes coordinateAxes(TransformHandle) const = 0;
  virtual void setTranslation(TransformHandle,Point) = 0;
  virtual Point translation(TransformHandle) const = 0;
  virtual void setColor(TransformHandle,float r,float g,float b) = 0;
  virtual void setStartPoint(LineHandle,Point) = 0;
  virtual void setEndPoint(LineHandle,Point) = 0;
  virtual Point worldPoint(Point local,TransformHandle) const = 0;
  virtual Optional<TransformHandle> selectedObject() const = 0;
  virtual void selectObject(TransformHandle) = 0;
  virtual Optional<LineHandle> maybeLine(TransformHandle) const = 0;
  virtual void attachDraggerToSelectedNode(DraggerType) = 0;

  TransformHandle createSphere() { return createSphere(top()); }
  TransformHandle createBox() { return createBox(top()); }

};

#endif /* SCENE_HPP_ */
