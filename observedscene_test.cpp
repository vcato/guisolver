#include "observedscene.hpp"

#include "faketreewidget.hpp"
#include "fakescene.hpp"
#include "checktree.hpp"
#include "startswith.hpp"
#include "scenestateio.hpp"
#include "vectorio.hpp"
#include "contains.hpp"

#define ADD_TEST 0

using std::cerr;
using VariableName = SceneState::Variable::Name;


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

  static void solveFunction(SceneState &)
  {
  }

  static void updateErrorsFunction(SceneState &)
  {
  }

  ObservedScene
    observed_scene{scene, tree_widget, updateErrorsFunction, solveFunction};
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


static void testDuplicatingABody()
{
  // Create a body that has a distance error
  //
  // * Marker global1
  // * Body
  //   * Marker local1
  //   * DistanceError local1 <-> global1
  //   * DistanceError global1 <-> local1

  Tester tester;
  ObservedScene &observed_scene = tester.observed_scene;
  SceneState &scene_state = observed_scene.scene_state;
  TreePaths &tree_paths = observed_scene.tree_paths;

  BodyIndex body1_index = observed_scene.addBody(/*parent*/{});
  MarkerIndex global_marker_index = observed_scene.addMarker(/*parent*/{});
  MarkerIndex local1_marker_index = observed_scene.addMarker(body1_index);

  observed_scene.addDistanceError(
    global_marker_index,
    local1_marker_index,
    body1_index
  );

  observed_scene.addDistanceError(
    local1_marker_index,
    global_marker_index,
    body1_index
  );

  // Duplicate the body
  BodyIndex body2_index = observed_scene.duplicateBody(body1_index);

  // * Marker global1
  // * Body
  //   * Marker local1
  //   * DistanceError local1 <-> global1
  //   * DistanceError global1 <-> local1
  // * Body
  //   * Marker local2
  //   * DistanceError local2 <-> global1
  //   * DistanceError global1 <-> local2
  assert(scene_state.bodies().size() == 2);

  MarkerIndex local2_marker_index = markersOnBody(body2_index, scene_state)[0];

  DistanceErrorIndex distance_error1_index =
    distanceErrorsOnBody(body2_index, scene_state)[0];

  SceneState::DistanceError &distance_error1_state =
    scene_state.distance_errors[distance_error1_index];

  assert(
    distance_error1_state.optional_start_marker_index == global_marker_index
  );

  assert(
    distance_error1_state.optional_end_marker_index == local2_marker_index
  );

  DistanceErrorIndex distance_error2_index =
    distanceErrorsOnBody(body2_index, scene_state)[1];

  SceneState::DistanceError &distance_error2_state =
    scene_state.distance_errors[distance_error2_index];

  assert(
    distance_error2_state.optional_end_marker_index == global_marker_index
  );

  assert(
    distance_error2_state.optional_start_marker_index == local2_marker_index
  );

  const TreePath &distance_error2_tree_path =
    tree_paths.distance_errors[distance_error1_index].path;

  const TreePath &body2_tree_path = tree_paths.body(body2_index).path;

  assert(startsWith(distance_error2_tree_path, body2_tree_path));

  assert(
    distance_error1_index
    < DistanceErrorIndex(observed_scene.scene_handles.distance_errors.size())
  );
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

  assert(tester.scene.maybe_dragger_type == Scene::ManipulatorType::translate);

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


static void testCutAndPaste()
{
  Tester tester;
  ObservedScene &observed_scene = tester.observed_scene;
  BodyIndex body_index = observed_scene.addBody(/*parent*/{});
  /*LineIndex line_index =*/ observed_scene.addLineTo(body_index);
  TreePaths &tree_paths = observed_scene.tree_paths;
  TreePaths old_paths = tree_paths;
  observed_scene.cutBody(body_index);
  observed_scene.pasteBodyGlobal({});
  tester.observed_scene.selectBody(body_index);
  checkTree(tester);
}


static void testAddingADistanceErrorToABody()
{
  Tester tester;
  ObservedScene &observed_scene = tester.observed_scene;
  BodyIndex body_index = observed_scene.addBody(/*parent*/{});

  DistanceErrorIndex distance_error_index =
    observed_scene.addDistanceError(/*start*/{}, /*end*/{}, body_index);

  const TreePath &distance_error_path =
    observed_scene.tree_paths.distance_errors[distance_error_index].path;

  const TreePath &body_path =
    observed_scene.tree_paths.body(body_index).path;

  assert(startsWith(distance_error_path, body_path));
}


static void testReplacingSceneState()
{
  // * Scene
  //   * Body
  //     * DistanceError

  Tester tester;
  ObservedScene &observed_scene = tester.observed_scene;
  SceneState new_scene_state;
  BodyIndex body_index = new_scene_state.createBody();
  new_scene_state.createDistanceError(body_index);
  observed_scene.replaceSceneStateWith(new_scene_state);
}


static void testAddingAVariable()
{
  Tester tester;
  ObservedScene &observed_scene = tester.observed_scene;
  observed_scene.addVariable();
  assert(observed_scene.scene_state.variables.size() == 1);
  TreePath expected_path = {0,0};
  assert(observed_scene.tree_paths.variables[0].path == expected_path);
  assert(observed_scene.scene_state.variables[0].name == "var1");
}


static void testUsingAVariable()
{
  Tester tester;
  ObservedScene &observed_scene = tester.observed_scene;
  SceneState &scene_state = observed_scene.scene_state;
  VariableIndex variable_index = observed_scene.addVariable();
  VariableName var_name = scene_state.variables[variable_index].name;
  TreePaths &tree_paths = observed_scene.tree_paths;
  BodyIndex body_index = observed_scene.addBody();

  TreePath variable_value_path =
    tree_paths.variables[variable_index].value;

  const TreePath tx_path = tree_paths.body(body_index).translation.x.path;
  observed_scene.handleTreeNumericValueChanged(variable_value_path, 4);
#if ADD_TEST
  assert(tester.tree_widget.item(tx_path).maybe_numeric_value == 4);
#endif

  observed_scene.handleTreeExpressionChanged(
    tree_paths.body(body_index).translation.x.path,
    /*expr*/var_name
  );

  assert(scene_state.body(body_index).expressions.translation.x == var_name);

  observed_scene.handleTreeNumericValueChanged(variable_value_path, 5);

  NumericValue tx_value =
    *tester.tree_widget.item(tx_path).maybe_numeric_value;

  assert(tx_value == 5);
}


static void testChangingMarkerName()
{
  Tester tester;
  ObservedScene &observed_scene = tester.observed_scene;
  TreePaths &tree_paths = observed_scene.tree_paths;
  MarkerIndex marker_index = observed_scene.addMarker();
  SceneState &scene_state = observed_scene.scene_state;
  const TreePath &name_path = tree_paths.marker(marker_index).name;
  observed_scene.handleTreeStringValueChanged(name_path, "new_name");
  assert(scene_state.marker(marker_index).name == "new_name");
}


static void testDuplicatingAMarkerWithDistanceError()
{
  Tester tester;
  ObservedScene &observed_scene = tester.observed_scene;
  TreePaths &tree_paths = observed_scene.tree_paths;
  FakeTreeWidget &tree_widget = tester.tree_widget;
  const SceneState &scene_state = observed_scene.scene_state;

  // Have a scene with a single marker and a distance error
  // with both markers being None.
  SceneState initial_state;
  MarkerIndex marker_index = initial_state.createMarker();
  MarkerIndex distance_error_index = initial_state.createDistanceError();
  observed_scene.replaceSceneStateWith(initial_state);

  // Duplicate the marker.
  MarkerIndex new_marker_index =
    observed_scene.duplicateMarkerWithDistanceError(marker_index);

  // Make sure that the start and end markers on the distance error in
  // the tree have options for the new marker.

  const TreePaths::DistanceError &distance_error_paths =
    tree_paths.distance_errors[distance_error_index];

  const FakeTreeItem::ValueString &start_marker_value_string =
    tree_widget.item(distance_error_paths.start).value_string;

  const FakeTreeItem::ValueString &end_marker_value_string =
    tree_widget.item(distance_error_paths.end).value_string;

  using MarkerName = SceneState::Marker::Name;

  const MarkerName &new_marker_name =
    scene_state.marker(new_marker_index).name;

  assert(contains(start_marker_value_string, new_marker_name));
  assert(contains(end_marker_value_string, new_marker_name));
}


static void testChangingSolveFlag()
{
  Tester tester;
  SceneState initial_state;
  BodyIndex body_index = initial_state.createBody();
  ObservedScene &observed_scene = tester.observed_scene;
  TreePaths &tree_paths = observed_scene.tree_paths;
  bool old_solve = true;
  initial_state.body(body_index).solve_flags.translation.x = old_solve;
  observed_scene.replaceSceneStateWith(initial_state);

  SceneState &scene_state = observed_scene.scene_state;
  FakeTreeItem::ValueString solve_value_string =
    tester.tree_widget.item(
      tree_paths.body(body_index).translation.x.solve_path
    ).value_string;

  assert(solve_value_string == "value=1");

  observed_scene.handleTreeBoolValueChanged(
    tree_paths.body(body_index).translation.x.solve_path, !old_solve
  );
  bool new_solve = scene_state.body(body_index).solve_flags.translation.x;
  assert(old_solve != new_solve);
}


static void testChangingSolvedValueInTree()
{
  SceneState initial_state;
  BodyIndex body_index = initial_state.createBody();
  Tester tester;
  ObservedScene &observed_scene = tester.observed_scene;
  observed_scene.replaceSceneStateWith(initial_state);
  TreePaths &tree_paths = observed_scene.tree_paths;
  const TreePaths::Body &body_paths = tree_paths.body(body_index);

  observed_scene.handleTreeNumericValueChanged(
    body_paths.translation.x.path, 2
  );
}


static void
expectAllSolveFlagsAreOn(
  const TreePaths::XYZChannels &xyz_paths,
  const FakeTreeWidget &tree_widget
)
{
  FakeTreeItem::ValueString on_value_string =
    tree_widget.boolValueText(true);

  const TreePath &x_solve_path = xyz_paths.x.solve_path;
  const TreePath &y_solve_path = xyz_paths.y.solve_path;
  const TreePath &z_solve_path = xyz_paths.z.solve_path;
  const FakeTreeItem &x_solve_item = tree_widget.item(x_solve_path);
  const FakeTreeItem &y_solve_item = tree_widget.item(y_solve_path);
  const FakeTreeItem &z_solve_item = tree_widget.item(z_solve_path);
  assert(x_solve_item.value_string == on_value_string);
  assert(y_solve_item.value_string == on_value_string);
  assert(z_solve_item.value_string == on_value_string);
}


static void testSetSolveFlags()
{
  SceneState initial_state;
  BodyIndex body_index = initial_state.createBody();
  Tester tester;
  ObservedScene &observed_scene = tester.observed_scene;
  observed_scene.replaceSceneStateWith(initial_state);
  using TreeItemDescription = ObservedScene::TreeItemDescription;
  {
    TreeItemDescription item;
    item.type = TreeItemDescription::Type::translation;
    item.maybe_body_index = body_index;
    observed_scene.setSolveFlags(item, true);
  }
  {
    TreeItemDescription item;
    item.type = TreeItemDescription::Type::rotation;
    item.maybe_body_index = body_index;
    observed_scene.setSolveFlags(item, true);
  }

  FakeTreeWidget &tree_widget = tester.tree_widget;
  const TreePaths &tree_paths = observed_scene.tree_paths;
  const TreePaths::Body &body_paths = tree_paths.body(body_index);
  expectAllSolveFlagsAreOn(body_paths.translation, tree_widget);
  expectAllSolveFlagsAreOn(body_paths.rotation, tree_widget);
}


int main()
{
  testTransferringABody1();
  testTransferringABody2();
  testTransferringAMarker();
  testDuplicatingABody();
  testDuplicatingABodyWithDistanceErrors();
  testUserSelectingABodyInTheTree();
  testSelectingSceneInTheTree();
  testSelectingMarkerInTheScene();
  testCutAndPaste();
  testAddingADistanceErrorToABody();
  testReplacingSceneState();
  testAddingAVariable();
  testChangingMarkerName();
  testDuplicatingAMarkerWithDistanceError();
  testChangingSolveFlag();
  testChangingSolvedValueInTree();
  testSetSolveFlags();
  testUsingAVariable();
}
