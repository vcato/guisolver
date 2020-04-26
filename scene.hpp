#ifndef SCENE_HPP_
#define SCENE_HPP_

#include <functional>
#include "coordinateaxes.hpp"
#include "optional.hpp"
#include "vector.hpp"
#include "mesh.hpp"

#define CHANGE_MANIPULATORS 0


struct Scene {
  using Point = Vec3;

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

  struct MeshHandle : GeometryHandle {
    using GeometryHandle::GeometryHandle;

    explicit MeshHandle(const GeometryHandle &arg)
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
  virtual MeshHandle createMesh(TransformHandle parent, const Mesh &) = 0 ;
  virtual TransformHandle parentTransform(GeometryHandle) const = 0;
#if CHANGE_MANIPULATORS
  virtual TransformHandle parentTransform(TransformHandle) const = 0;
#endif
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
  virtual void setMesh(MeshHandle, Mesh) = 0;
  virtual const Mesh& mesh(MeshHandle) const = 0;
  virtual void setLineStartPoint(LineHandle, Point) = 0;
  virtual void setLineEndPoint(LineHandle, Point) = 0;
  virtual Optional<GeometryHandle> selectedGeometry() const = 0;
  virtual Optional<TransformHandle> selectedTransform() const = 0;
  virtual void selectGeometry(GeometryHandle) = 0;
  virtual void selectTransform(TransformHandle) = 0;
  virtual Optional<LineHandle> maybeLine(GeometryHandle) const = 0;
#if !CHANGE_MANIPULATORS
  virtual void attachManipulatorToSelectedNode(ManipulatorType) = 0;
#else
  virtual TransformHandle
    createTranslateManipulator(TransformHandle parent) = 0;

  virtual GeometryHandle
    createScaleManipulator(TransformHandle parent) = 0;
#endif
};


inline std::ostream&
operator<<(std::ostream &stream, const Scene::GeometryHandle &arg)
{
  stream << "Scene::GeometryHandle(index=" << arg.index << ")";
  return stream;
}


#endif /* SCENE_HPP_ */
