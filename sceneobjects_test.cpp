#include "sceneobjects.hpp"

#include "transformstate.hpp"
#include "fakescene.hpp"


static Scene::Point
sceneGeometryCenter(
  BodyIndex index,
  const FakeScene &scene,
  const SceneHandles &scene_handles
)
{
  const SceneHandles::Body &body_handles = scene_handles.body(index);
  assert(body_handles.boxes.size() == 1);

  const FakeScene::Objects::const_iterator iter =
    scene.objects.find(body_handles.boxes[0].handle.index);

  assert(iter != scene.objects.end());
  return *iter->second.maybe_geometry_center;
}


int main()
{
  FakeScene scene;

  Scene::Point center = { 1.5, 2.5, 3.5};
  SceneState state;
  BodyIndex parent_index = state.createBody(/*parent*/{});
  state.body(parent_index).addBox();
  BodyIndex child_index = state.createBody(parent_index);
  state.body(child_index).addBox();
  state.body(child_index).boxes[0].center = xyzStateFromVec3(center);
  SceneHandles scene_handles = createSceneObjects(state, scene);
  assert(sceneGeometryCenter(child_index, scene, scene_handles) == center);
  destroySceneObjects(scene, state, scene_handles);
  assert(scene.objects.empty());
}
