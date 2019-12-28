#include "sceneobjects.hpp"

#include <map>

#define ADD_TEST 1


using std::map;


namespace {
struct FakeScene : Scene {
  using TransformIndex = TransformHandle::Index;
  TransformHandle top_handle = {0};
  map<TransformIndex, TransformIndex> parent_map;

  bool indexIsUsed(TransformIndex i) const
  {
    return parent_map.count(i) != 0;
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

    for (auto item : parent_map) {
      TransformIndex next_parent_index = item.second;

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

  virtual TransformHandle createSphere(TransformHandle)
  {
    assert(false); // not needed
  }

  virtual TransformHandle createBox(TransformHandle parent)
  {
    TransformHandle new_handle = {firstUnusedIndex()};
    parent_map[new_handle.index] = parent.index;
    return new_handle;
  }

  virtual LineHandle createLine(TransformHandle)
  {
    assert(false); // not needed
  }

  virtual void destroyLine(LineHandle)
  {
    assert(false); // not needed
  }

  virtual void destroyObject(TransformHandle handle)
  {
    assert(nChildren(handle) == 0);
    parent_map.erase(handle.index);
  }

  virtual void
  setGeometryScale(TransformHandle,float /*x*/,float /*y*/,float /*z*/)
  {
  }

  virtual Vec3 geometryScale(TransformHandle) const
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
    assert(false); // not needed
  }

  virtual void setStartPoint(LineHandle,Point)
  {
    assert(false); // not needed
  }

  virtual void setEndPoint(LineHandle,Point)
  {
    assert(false); // not needed
  }

  virtual Point worldPoint(Point /*local*/,TransformHandle) const
  {
    assert(false); // not needed
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
};
}


int main()
{
  FakeScene scene;

  SceneState state;
  BodyIndex parent_index = state.createBody(/*parent*/{});
  /*BodyIndex child_index =*/ state.createBody(parent_index);
  SceneHandles scene_handles = createSceneObjects(state, scene);
  destroySceneObjects(scene, state, scene_handles);
  assert(scene.parent_map.empty());
}
