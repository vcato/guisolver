#include "observedscene.hpp"

#include "faketreewidget.hpp"
#include "fakescene.hpp"
#include "checktree.hpp"

using std::cerr;


static void
transferBody(
  BodyIndex body_index,
  Optional<BodyIndex> maybe_new_parent_index,
  ObservedScene &observed_scene
)
{
  observed_scene.cutBody(body_index);
  observed_scene.pasteBodyGlobal(maybe_new_parent_index);
}


static void
transferMarker(
  MarkerIndex marker_index,
  Optional<BodyIndex> maybe_new_parent_index,
  ObservedScene &observed_scene
)
{
  observed_scene.cutMarker(marker_index);
  observed_scene.pasteMarkerGlobal(maybe_new_parent_index);
}


namespace {
struct Tester {
  FakeTreeWidget tree_widget;
  FakeScene scene;
  ObservedScene observed_scene{scene, tree_widget, [](SceneState &){}};
};
}


static void checkTree(Tester &tester)
{
  checkTree(
    tester.tree_widget,
    tester.observed_scene.tree_paths,
    tester.observed_scene.scene_state
  );
}


static void testTransferringABody1()
{
  Tester tester;
  ObservedScene &observed_scene = tester.observed_scene;

  BodyIndex body1_index = observed_scene.addBody(/*parent*/{});
  BodyIndex body2_index = observed_scene.addBody(/*parent*/{});

  observed_scene.addMarker(body2_index);
  transferBody(body1_index, body2_index, observed_scene);
  checkTree(tester);
}


static void testTransferringABody2()
{
  Tester tester;
  ObservedScene &observed_scene = tester.observed_scene;
  BodyIndex body1_index = observed_scene.addBody(/*parent*/{});
  observed_scene.addMarker(body1_index);
  transferBody(body1_index, {}, observed_scene);
  checkTree(tester);
}


static void testTransferringAMarker()
{
  Tester tester;
  ObservedScene &observed_scene = tester.observed_scene;
  BodyIndex body1_index = observed_scene.addBody(/*parent*/{});
  BodyIndex body2_index = observed_scene.addBody(/*parent*/{});
  MarkerIndex marker_index = observed_scene.addMarker(body1_index);
  transferMarker(marker_index, body2_index, observed_scene);

  assert(
    observed_scene.scene_state.marker(marker_index).maybe_body_index
    == body2_index
  );

  checkTree(tester);
}


static void testDuplicatingABodyWithDistanceErrors()
{
  Tester tester;
  ObservedScene &observed_scene = tester.observed_scene;
  BodyIndex body_index = observed_scene.addBody(/*parent*/{});
  MarkerIndex marker_index = observed_scene.addMarker(body_index);
  SceneState &scene_state = observed_scene.scene_state;

  BodyIndex new_body_index =
    observed_scene.duplicateBodyWithDistanceErrors(body_index);

  assert(scene_state.distance_errors.size() == 1);

  const SceneState::DistanceError &distance_error_state =
    scene_state.distance_errors[0];

  assert(distance_error_state.optional_start_marker_index == marker_index);
  assert(distance_error_state.optional_end_marker_index.hasValue());
  assert(scene_state.bodies().size() == 2);
  assert(new_body_index != body_index);
  checkTree(tester);
}


static void testUserSelectingABodyInTheTree()
{
  Tester tester;
  ObservedScene &observed_scene = tester.observed_scene;
  BodyIndex body_index = observed_scene.addBody(/*parent*/{});
  SceneState &scene_state = observed_scene.scene_state;

  assert(scene_state.bodies().size() == 1);
  { // User selects the body item in the tree.
    tester.tree_widget.maybe_selected_item =
      observed_scene.tree_paths.body(body_index).path;

    observed_scene.handleTreeSelectionChanged();
  }

  assert(tester.scene.maybe_dragger_type == Scene::DraggerType::translate);

  assert(
    tester.scene.maybe_dragger_index
    == observed_scene.scene_handles.body(body_index).transform_handle.index
  );
}


static void testSelectingSceneInTheTree()
{
  Tester tester;
  SceneState initial_scene_state;
  initial_scene_state.createMarker(/*parent*/Optional<BodyIndex>{});
  ObservedScene &observed_scene = tester.observed_scene;
  observed_scene.replaceSceneStateWith(initial_scene_state);
  tester.tree_widget.maybe_selected_item = observed_scene.tree_paths.path;
  observed_scene.handleTreeSelectionChanged();
}


static void testSelectingMarkerInTheScene()
{
  Tester tester;
  SceneState initial_scene_state;
  ObservedScene &observed_scene = tester.observed_scene;

  MarkerIndex marker_index =
    initial_scene_state.createMarker(/*parent*/Optional<BodyIndex>{});

  observed_scene.replaceSceneStateWith(initial_scene_state);

  tester.scene.maybe_selected_object_index =
    observed_scene.scene_handles.marker(marker_index).sphereHandle().index;

  observed_scene.handleSceneSelectionChanged();

  assert(tester.tree_widget.maybe_selected_item ==
    observed_scene.tree_paths.marker(marker_index).path);
}


int main()
{
  testTransferringABody1();
  testTransferringABody2();
  testTransferringAMarker();
  testDuplicatingABodyWithDistanceErrors();
  testUserSelectingABodyInTheTree();
  testSelectingSceneInTheTree();
  testSelectingMarkerInTheScene();
}
