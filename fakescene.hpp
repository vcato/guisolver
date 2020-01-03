#include <map>
#include "scene.hpp"


struct FakeScene : Scene {
  using TransformIndex = TransformHandle::Index;
  TransformHandle top_handle = {0};

  struct Body {
    TransformIndex parent_index;
    Point geometry_center = {0,0,0};
  };

  using Bodies = std::map<TransformIndex, Body>;
  Bodies bodies;

  bool indexIsUsed(TransformIndex i) const
  {
    return bodies.count(i) != 0;
  }

  TransformIndex firstUnusedIndex() const
  {
    TransformIndex i = 1;

    while (indexIsUsed(i)) {
      ++i;
    }

    return i;
  }

  int nChildren(TransformHandle handle) const
  {
    int count = 0;

    for (auto item : bodies) {
      TransformIndex next_parent_index = item.second.parent_index;

      if (next_parent_index == handle.index) {
        ++count;
      }
    }

    return count;
  }

  virtual TransformHandle top() const
  {
    return top_handle;
  }

  virtual SphereAndTransformHandle
    createSphereAndTransform(TransformHandle parent_handle);

  virtual BoxAndTransformHandle createBoxAndTransform(TransformHandle parent);
  virtual LineAndTransformHandle createLineAndTransform(TransformHandle);

  virtual void destroyTransformAndGeometry(GeometryAndTransformHandle handle)
  {
    assert(nChildren(handle.transform_handle) == 0);
    bodies.erase(handle.transform_handle.index);
  }

  void setGeometryScale(GeometryAndTransformHandle,const Vec3 &) override
  {
  }

  void
  setGeometryCenter(
    GeometryAndTransformHandle handle,const Point &center
  ) override
  {
    bodies[handle.transform_handle.index].geometry_center = center;
  }

  virtual Vec3 geometryScale(GeometryAndTransformHandle) const
  {
    assert(false); // not needed
  }

  virtual Point geometryCenter(GeometryAndTransformHandle) const
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

  virtual void selectObject(GeometryAndTransformHandle)
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
    GeometryAndTransformHandle create(TransformHandle parent_handle);
};
