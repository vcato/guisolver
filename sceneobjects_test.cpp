#include "sceneobjects.hpp"

#include <map>
#include "transformstate.hpp"

using std::map;


namespace {
struct FakeScene : Scene {
  using TransformIndex = TransformHandle::Index;
  TransformHandle top_handle = {0};

  struct Body {
    TransformIndex parent_index;
    Vec3 geometry_center = {0,0,0};
  };

  using Bodies = map<TransformIndex, Body>;
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

  virtual TransformHandle createSphere(TransformHandle)
  {
    assert(false); // not needed
  }

  virtual TransformHandle createBox(TransformHandle parent)
  {
    TransformHandle new_handle = {firstUnusedIndex()};
    bodies[new_handle.index].parent_index = parent.index;
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
    bodies.erase(handle.index);
  }

  void setGeometryScale(TransformHandle,const Vec3 &) override
  {
  }

  void setGeometryCenter(TransformHandle handle,const Vec3 &center) override
  {
    bodies[handle.index].geometry_center = center;
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


static Vec3
sceneGeometryCenter(
  BodyIndex index,
  const FakeScene &scene,
  const SceneHandles &scene_handles
)
{
  const FakeScene::Bodies::const_iterator iter =
    scene.bodies.find(scene_handles.bodies[index].index);

  assert(iter != scene.bodies.end());
  return iter->second.geometry_center;
}


int main()
{
  FakeScene scene;

  Vec3 center = { 1.5, 2.5, 3.5};
  SceneState state;
  BodyIndex parent_index = state.createBody(/*parent*/{});
  BodyIndex child_index = state.createBody(parent_index);
  state.body(child_index).geometry.center = xyzState(center);
  SceneHandles scene_handles = createSceneObjects(state, scene);
  assert(sceneGeometryCenter(child_index, scene, scene_handles) == center);
  destroySceneObjects(scene, state, scene_handles);
  assert(scene.bodies.empty());
}
