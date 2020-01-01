#include "sceneobjects.hpp"

#include <map>
#include "transformstate.hpp"
#include "fakescene.hpp"

using std::map;


static Point
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

  Point center = { 1.5, 2.5, 3.5};
  SceneState state;
  BodyIndex parent_index = state.createBody(/*parent*/{});
  BodyIndex child_index = state.createBody(parent_index);
  state.body(child_index).geometry.center = xyzState(center);
  SceneHandles scene_handles = createSceneObjects(state, scene);
  assert(sceneGeometryCenter(child_index, scene, scene_handles) == center);
  destroySceneObjects(scene, state, scene_handles);
  assert(scene.bodies.empty());
}
