#include <map>
#include "scene.hpp"


struct FakeScene : Scene {
  using TransformIndex = TransformHandle::Index;
  TransformHandle top_handle = {0};

  struct Object {
    TransformIndex parent_index;
    Point geometry_center = {0,0,0};
  };

  using Objects = std::map<TransformIndex, Object>;
  Objects objects;

  bool indexIsUsed(TransformIndex i) const
  {
    return objects.count(i) != 0;
  }

  TransformIndex firstUnusedIndex() const
  {
    TransformIndex i = 1;

    while (indexIsUsed(i)) {
      ++i;
    }

    return i;
  }

  virtual TransformHandle top() const
  {
    return top_handle;
  }

  SphereAndTransformHandle
    createSphereAndTransform(TransformHandle parent_handle) override;

  GeometryHandle createBox(TransformHandle parent) override;
  TransformHandle createTransform(TransformHandle parent) override;
  LineAndTransformHandle createLineAndTransform(TransformHandle) override;
  void destroyGeometry(GeometryHandle) override;
  void destroyTransform(TransformHandle) override;

  void setGeometryScale(GeometryHandle,const Vec3 &) override
  {
  }

  void
  setGeometryCenter(
    GeometryHandle geometry_handle,const Point &center
  ) override
  {
    objects[geometry_handle.index].geometry_center = center;
  }

  virtual Vec3 geometryScale(GeometryAndTransformHandle) const
  {
    assert(false); // not needed
  }

  virtual Vec3 geometryScale(GeometryHandle) const
  {
    assert(false); // not needed
  }

  virtual Point geometryCenter(GeometryAndTransformHandle) const
  {
    assert(false); // not needed
  }

  virtual Point geometryCenter(GeometryHandle) const
  {
    assert(false); // not needed
  }

  virtual void setCoordinateAxes(TransformHandle,const CoordinateAxes &)
  {
  }

  virtual CoordinateAxes coordinateAxes(TransformHandle) const
  {
    assert(false); // not needed
  }

  virtual void setTranslation(TransformHandle,Point)
  {
  }

  virtual Point translation(TransformHandle) const
  {
    assert(false); // not needed
  }

  virtual void
  setGeometryColor(
    GeometryAndTransformHandle,float /*r*/,float /*g*/,float /*b*/
  )
  {
  }

  virtual void setStartPoint(LineAndTransformHandle,Point)
  {
  }

  virtual void setEndPoint(LineAndTransformHandle,Point)
  {
  }

  virtual Optional<GeometryAndTransformHandle> selectedObject() const
  {
    assert(false); // not needed
  }

  virtual void selectGeometry(GeometryHandle)
  {
    assert(false); // not needed
  }

  virtual Optional<LineAndTransformHandle>
  maybeLineAndTransform(GeometryAndTransformHandle) const
  {
    assert(false); // not needed
  }

  virtual void attachDraggerToSelectedNode(DraggerType)
  {
    assert(false); // not needed
  }

  private:
    GeometryHandle createGeometry(TransformHandle parent);

    GeometryAndTransformHandle
      createGeometryAndTransform(TransformHandle parent_handle);

    int nChildren(size_t handle_index) const;
};
