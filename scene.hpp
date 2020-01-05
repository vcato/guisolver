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

  struct GeometryHandle {
    size_t index;

    GeometryHandle(size_t index) : index(index) {}

    bool operator==(const GeometryHandle &arg) const
    {
      return index == arg.index;
    }
  };

  struct LineHandle : GeometryHandle {
    using GeometryHandle::GeometryHandle;

    explicit LineHandle(const GeometryHandle &arg)
    : GeometryHandle(arg)
    {
    }
  };

  struct GeometryAndTransformHandle {
    TransformHandle transform_handle;
    GeometryHandle geometry_handle;

    GeometryAndTransformHandle(
      TransformHandle transform_handle,
      GeometryHandle geometry_handle
    )
    : transform_handle(transform_handle),
      geometry_handle(geometry_handle)
    {
    }

    bool operator==(const GeometryAndTransformHandle &arg) const
    {
      return
        transform_handle == arg.transform_handle &&
        geometry_handle == arg.geometry_handle;
    }
  };

  struct BoxAndTransformHandle : GeometryAndTransformHandle {
    explicit BoxAndTransformHandle(const GeometryAndTransformHandle &arg)
    : GeometryAndTransformHandle(arg)
    {
    }
  };

  struct SphereAndTransformHandle : GeometryAndTransformHandle {
    explicit SphereAndTransformHandle(const GeometryAndTransformHandle &arg)
    : GeometryAndTransformHandle(arg)
    {
    }
  };

  std::function<void()> changing_callback;
  std::function<void()> changed_callback;
  std::function<void()> selection_changed_callback;

  virtual TransformHandle top() const = 0;
  virtual TransformHandle createTransform(TransformHandle parent) = 0;
  virtual GeometryHandle createBox(TransformHandle parent) = 0;
  virtual LineHandle createLine(TransformHandle parent) = 0;

  virtual SphereAndTransformHandle
    createSphereAndTransform(TransformHandle parent) = 0;

  virtual TransformHandle parentTransform(GeometryHandle) const = 0;
  virtual void destroyGeometry(GeometryHandle) = 0;
  virtual void destroyTransform(TransformHandle) = 0;

  virtual void setGeometryScale(GeometryHandle handle,const Vec3 &v) = 0;
  virtual void setGeometryCenter(GeometryHandle handle,const Point &v) = 0;
  virtual Vec3 geometryScale(GeometryHandle) const = 0;
  virtual Point geometryCenter(GeometryHandle) const = 0;
  virtual void setCoordinateAxes(TransformHandle,const CoordinateAxes &) = 0;
  virtual CoordinateAxes coordinateAxes(TransformHandle) const = 0;
  virtual void setTranslation(TransformHandle,Point) = 0;
  virtual Point translation(TransformHandle) const = 0;

  virtual void
    setGeometryColor(GeometryHandle handle,float r,float g,float b) = 0;

  void
  setGeometryColor(
    GeometryAndTransformHandle handle,float r,float g,float b
  )
  {
    setGeometryColor(handle.geometry_handle, r,g,b);
  }

  virtual void setStartPoint(LineHandle,Point) = 0;
  virtual void setEndPoint(LineHandle,Point) = 0;
  virtual Optional<GeometryHandle> selectedGeometry() const = 0;
  virtual Optional<TransformHandle> selectedTransform() const = 0;
  virtual void selectGeometry(GeometryHandle) = 0;
  virtual void selectTransform(TransformHandle) = 0;
  virtual Optional<LineHandle> maybeLine(GeometryHandle) const = 0;
  virtual void attachDraggerToSelectedNode(DraggerType) = 0;

  SphereAndTransformHandle createSphereAndTransform()
  {
    return createSphereAndTransform(top());
  }
};


inline std::ostream&
operator<<(std::ostream &stream, const Scene::GeometryHandle &arg)
{
  stream << "Scene::GeometryHandle(index=" << arg.index << ")";
  return stream;
}


#endif /* SCENE_HPP_ */
