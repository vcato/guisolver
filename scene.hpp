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

  struct LineAndTransformHandle {
    TransformHandle transform_handle;
    LineAndTransformHandle(size_t index) : transform_handle{index} {}
  };

  struct BoxAndTransformHandle {
    TransformHandle transform_handle;
    BoxAndTransformHandle(size_t index) : transform_handle{index} {}
  };

  struct SphereAndTransformHandle {
    TransformHandle transform_handle;
    SphereAndTransformHandle(size_t index) : transform_handle{index} {}
  };

  std::function<void()> changing_callback;
  std::function<void()> changed_callback;
  std::function<void()> selection_changed_callback;

  virtual TransformHandle top() const = 0;

  virtual SphereAndTransformHandle
    createSphereAndTransform(TransformHandle parent) = 0;

  virtual BoxAndTransformHandle
    createBoxAndTransform(TransformHandle parent) = 0;

  virtual LineAndTransformHandle
    createLineAndTransform(TransformHandle parent) = 0;

  virtual void destroyLineAndTransform(LineAndTransformHandle) = 0;
  virtual void destroyObject(TransformHandle) = 0;
  virtual void setGeometryScale(TransformHandle, const Vec3 &) = 0;
  virtual void setGeometryCenter(TransformHandle, const Point &) = 0;
  virtual Vec3 geometryScale(TransformHandle) const = 0;
  virtual Point geometryCenter(TransformHandle) const = 0;
  virtual void setCoordinateAxes(TransformHandle,const CoordinateAxes &) = 0;
  virtual CoordinateAxes coordinateAxes(TransformHandle) const = 0;
  virtual void setTranslation(TransformHandle,Point) = 0;
  virtual Point translation(TransformHandle) const = 0;
  virtual void setColor(TransformHandle,float r,float g,float b) = 0;
  virtual void setStartPoint(LineAndTransformHandle,Point) = 0;
  virtual void setEndPoint(LineAndTransformHandle,Point) = 0;
  virtual Optional<TransformHandle> selectedObject() const = 0;
  virtual void selectObject(TransformHandle) = 0;

  virtual Optional<LineAndTransformHandle>
    maybeLineAndTransform(TransformHandle) const = 0;

  virtual void attachDraggerToSelectedNode(DraggerType) = 0;

  SphereAndTransformHandle createSphereAndTransform()
  {
    return createSphereAndTransform(top());
  }

  BoxAndTransformHandle createBoxAndTransform()
  {
    return createBoxAndTransform(top());
  }
};

#endif /* SCENE_HPP_ */
