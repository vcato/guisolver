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

  struct GeometryAndTransformHandle {
    TransformHandle transform_handle;

    GeometryAndTransformHandle(size_t index)
    : transform_handle{index}
    {
    }

    bool operator==(const GeometryAndTransformHandle &arg) const
    {
      return transform_handle == arg.transform_handle;
    }
  };

  struct LineAndTransformHandle : GeometryAndTransformHandle {
    LineAndTransformHandle(size_t index)
    : GeometryAndTransformHandle{index} {}

    explicit LineAndTransformHandle(const GeometryAndTransformHandle &arg)
    : GeometryAndTransformHandle(arg)
    {
    }
  };

  struct BoxAndTransformHandle : GeometryAndTransformHandle {
    BoxAndTransformHandle(size_t index)
    : GeometryAndTransformHandle{index} {}

    explicit BoxAndTransformHandle(const GeometryAndTransformHandle &arg)
    : GeometryAndTransformHandle(arg)
    {
    }
  };

  struct SphereAndTransformHandle : GeometryAndTransformHandle {
    SphereAndTransformHandle(size_t index)
    : GeometryAndTransformHandle{index} {}

    explicit SphereAndTransformHandle(const GeometryAndTransformHandle &arg)
    : GeometryAndTransformHandle(arg)
    {
    }
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

  virtual void destroyTransformAndGeometry(GeometryAndTransformHandle) = 0;
  virtual void setGeometryScale(GeometryAndTransformHandle, const Vec3 &) = 0;
  virtual void setGeometryCenter(GeometryAndTransformHandle, const Point &) = 0;
  virtual Vec3 geometryScale(GeometryAndTransformHandle) const = 0;
  virtual Point geometryCenter(GeometryAndTransformHandle) const = 0;
  virtual void setCoordinateAxes(TransformHandle,const CoordinateAxes &) = 0;
  virtual CoordinateAxes coordinateAxes(TransformHandle) const = 0;
  virtual void setTranslation(TransformHandle,Point) = 0;
  virtual Point translation(TransformHandle) const = 0;

  virtual void
    setGeometryColor(GeometryAndTransformHandle,float r,float g,float b) = 0;

  virtual void setStartPoint(LineAndTransformHandle,Point) = 0;
  virtual void setEndPoint(LineAndTransformHandle,Point) = 0;
  virtual Optional<GeometryAndTransformHandle> selectedObject() const = 0;
  virtual void selectObject(GeometryAndTransformHandle) = 0;

  virtual Optional<LineAndTransformHandle>
    maybeLineAndTransform(GeometryAndTransformHandle) const = 0;

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
