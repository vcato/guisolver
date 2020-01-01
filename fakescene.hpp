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

  virtual TransformHandle createSphere(TransformHandle parent_handle);

  virtual TransformHandle createBox(TransformHandle parent);

  virtual LineHandle createLine(TransformHandle);

  virtual void destroyLine(LineHandle)
  {
    assert(false); // not needed
  }

  virtual void destroyObject(TransformHandle handle)
  {
    assert(nChildren(handle) == 0);
    bodies.erase(handle.index);
  }

  void setGeometryScale(TransformHandle,const Vec3 &) override
  {
  }

  void setGeometryCenter(TransformHandle handle,const Point &center) override
  {
    bodies[handle.index].geometry_center = center;
  }

  virtual Vec3 geometryScale(TransformHandle) const
  {
    assert(false); // not needed
  }

  virtual Point geometryCenter(TransformHandle) const
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

  virtual void setColor(TransformHandle,float /*r*/,float /*g*/,float /*b*/)
  {
  }

  virtual void setStartPoint(LineHandle,Point)
  {
  }

  virtual void setEndPoint(LineHandle,Point)
  {
  }

  virtual Optional<TransformHandle> selectedObject() const
  {
    assert(false); // not needed
  }

  virtual void selectObject(TransformHandle)
  {
    assert(false); // not needed
  }

  virtual Optional<LineHandle> maybeLine(TransformHandle) const
  {
    assert(false); // not needed
  }

  virtual void attachDraggerToSelectedNode(DraggerType)
  {
    assert(false); // not needed
  }

  private:
    TransformHandle create(TransformHandle parent_handle);
};
