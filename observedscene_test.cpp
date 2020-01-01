#include "observedscene.hpp"

#include "faketreewidget.hpp"
#include "fakescene.hpp"
#include "checktree.hpp"

using std::cerr;


static void
transferBody(
  BodyIndex body1_index,
  Optional<BodyIndex> maybe_new_parent_index,
  ObservedScene &observed_scene
)
{
  ObservedScene::cutBody(observed_scene, body1_index);
  ObservedScene::pasteGlobal(maybe_new_parent_index, observed_scene);
}


static void testCutAndPaste1()
{
  FakeTreeWidget tree_widget;
  FakeScene scene;
  ObservedScene observed_scene(scene, tree_widget);

  BodyIndex body1_index =
    ObservedScene::addBody(/*parent*/{}, observed_scene);

  BodyIndex body2_index =
    ObservedScene::addBody(/*parent*/{}, observed_scene);

  ObservedScene::addMarker(observed_scene, body2_index);
  transferBody(body1_index, body2_index, observed_scene);

  const SceneState &scene_state = observed_scene.scene_state;
  checkTree(tree_widget, observed_scene.tree_paths, scene_state);
}


static void testCutAndPaste2()
{
  FakeTreeWidget tree_widget;
  FakeScene scene;
  ObservedScene observed_scene(scene, tree_widget);

  BodyIndex body1_index =
    ObservedScene::addBody(/*parent*/{}, observed_scene);

  ObservedScene::addMarker(observed_scene, body1_index);
  transferBody(body1_index, {}, observed_scene);
  const SceneState &scene_state = observed_scene.scene_state;
  checkTree(tree_widget, observed_scene.tree_paths, scene_state);
}


int main()
{
  testCutAndPaste1();
  testCutAndPaste2();
}
