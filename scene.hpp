#ifndef SCENE_HPP_
#define SCENE_HPP_

#include <functional>
#include "coordinateaxes.hpp"
#include "optional.hpp"


struct Scene {
  using Point = Vec3;
  using Vector = Vec3;

  enum class ManipulatorType {
    translate,
    rotate,
    scale
  };

  struct Color {
    float red;
    float green;
    float blue;
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

  std::function<void()> changing_callback;
  std::function<void()> changed_callback;
  std::function<void()> selection_changed_callback;

  virtual TransformHandle top() const = 0;
  virtual TransformHandle createTransform(TransformHandle parent) = 0;
  virtual GeometryHandle createBox(TransformHandle parent) = 0;
  virtual LineHandle createLine(TransformHandle parent) = 0;
  virtual GeometryHandle createSphere(TransformHandle parent) = 0;
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
  virtual void setGeometryColor(GeometryHandle, const Color &) = 0;
  virtual void setStartPoint(LineHandle,Point) = 0;
  virtual void setEndPoint(LineHandle,Point) = 0;
  virtual Optional<GeometryHandle> selectedGeometry() const = 0;
  virtual Optional<TransformHandle> selectedTransform() const = 0;
  virtual void selectGeometry(GeometryHandle) = 0;
  virtual void selectTransform(TransformHandle) = 0;
  virtual Optional<LineHandle> maybeLine(GeometryHandle) const = 0;
  virtual void attachManipulatorToSelectedNode(ManipulatorType) = 0;
};


inline std::ostream&
operator<<(std::ostream &stream, const Scene::GeometryHandle &arg)
{
  stream << "Scene::GeometryHandle(index=" << arg.index << ")";
  return stream;
}


#endif /* SCENE_HPP_ */
