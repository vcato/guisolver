#include "observedscene.hpp"

#include "faketreewidget.hpp"
#include "fakescene.hpp"
#include "checktree.hpp"

using std::cerr;


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
  Clipboard clipboard;
  ObservedScene::cutBody(observed_scene, body1_index, clipboard);

  ObservedScene::pasteGlobal(body2_index, clipboard, observed_scene);

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
  Clipboard clipboard;
  ObservedScene::cutBody(observed_scene, body1_index, clipboard);

  ObservedScene::pasteGlobal(/*parent*/{}, clipboard, observed_scene);

  const SceneState &scene_state = observed_scene.scene_state;
  checkTree(tree_widget, observed_scene.tree_paths, scene_state);
}


int main()
{
  testCutAndPaste1();
  testCutAndPaste2();
}
