#include "observedscene.hpp"

#include "faketreewidget.hpp"
#include "fakescene.hpp"
#include "checktree.hpp"


int main()
{
  FakeTreeWidget tree_widget;
  FakeScene scene;
  ObservedScene obsererved_scene(scene, tree_widget);

  BodyIndex body1_index =
    ObservedScene::addBody(/*parent*/{}, obsererved_scene);

  BodyIndex body2_index =
    ObservedScene::addBody(/*parent*/{}, obsererved_scene);

  ObservedScene::addMarker(obsererved_scene, body2_index);
  Clipboard clipboard;
  ObservedScene::cutBody(obsererved_scene, body1_index, clipboard);
  --body2_index;

  ObservedScene::pasteGlobal(body2_index, clipboard, obsererved_scene);

  const SceneState &scene_state = obsererved_scene.scene_state;
  checkTree(tree_widget, obsererved_scene.tree_paths, scene_state);
}
