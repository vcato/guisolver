#include "observedscene.hpp"

#include "faketreewidget.hpp"
#include "fakescene.hpp"
#include "checktree.hpp"
#include "startswith.hpp"
#include "scenestateio.hpp"
#include "vectorio.hpp"
#include "contains.hpp"
#include "assertnearfloat.hpp"

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


static void testDuplicateBody()
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


static void testDuplicateBodyWhenTheBodyHasExpressions()
{
  SceneState initial_state;
  BodyIndex body_index = initial_state.createBody();
  BoxIndex box_index = initial_state.body(body_index).createBox();
  Expression test_expression = "1+2";

  initial_state.body(body_index)
    .expressions
    .translation
    .x = test_expression;

  initial_state.body(body_index)
    .expressions
    .rotation
    .x = test_expression;

  initial_state.body(body_index).boxes[box_index].center_expressions.x =
    test_expression;

  Tester tester;
  ObservedScene &observed_scene = tester.observed_scene;
  observed_scene.replaceSceneStateWith(initial_state);
  BodyIndex duplicate_body_index = observed_scene.duplicateBody(body_index);

  // check that the duplicate has the same expression
  const Expression &duplicate_expression =
    observed_scene
    .scene_state
    .body(duplicate_body_index)
    .expressions
    .translation
    .x;

  assert(duplicate_expression == test_expression);
  TreePaths &tree_paths = observed_scene.tree_paths;

  TreePath tx_path =
    tree_paths.body(duplicate_body_index).translation.x.path;

  TreePath rx_path =
    tree_paths.body(duplicate_body_index).rotation.x.path;

  TreePath centerx_path =
    tree_paths.body(duplicate_body_index).boxes[box_index].center.x;

  TreeWidget::Input tx_input = tester.tree_widget.item(tx_path).input;
  TreeWidget::Input rx_input = tester.tree_widget.item(rx_path).input;

  TreeWidget::Input centerx_input =
    tester.tree_widget.item(centerx_path).input;

  assert(tx_input == "=" + test_expression);
  assert(rx_input == "=" + test_expression);
  assert(centerx_input == "=" + test_expression);
}


static void testDuplicateBodyWithDistanceErrors()
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


static void
userChangesTreeItemExpression(
  const TreePath &channel_path,
  const Expression &expr,
  Tester &tester
)
{
  ObservedScene &observed_scene = tester.observed_scene;
  observed_scene.handleTreeExpressionChanged(channel_path, expr);
}


static void
userChangesVariableValue(
  VariableIndex variable_index,
  NumericValue new_value,
  Tester &tester
)
{
  ObservedScene &observed_scene = tester.observed_scene;
  TreePaths &tree_paths = observed_scene.tree_paths;

  TreePath variable_value_path =
    tree_paths.variables[variable_index].valuePath();

  tester.tree_widget.item(variable_value_path).maybe_numeric_value = new_value;
  observed_scene.handleTreeNumericValueChanged(variable_value_path, new_value);
}


static void testAddingAndRemovingAVariable()
{
  Tester tester;
  ObservedScene &observed_scene = tester.observed_scene;
  VariableIndex variable_index = observed_scene.addVariable();
  TreePaths &tree_paths = observed_scene.tree_paths;
  FakeTreeWidget &tree_widget = tester.tree_widget;
  assert(observed_scene.scene_state.variables.size() == 1);
  TreePath expected_path = {0,0};
  const TreePath &variable_path = tree_paths.variables[variable_index].path;
  assert(variable_path == expected_path);
  assert(observed_scene.scene_state.variables[variable_index].name == "var1");

  assert(
    observed_scene.describePath(variable_path).type
    == ObservedScene::TreeItemDescription::Type::variable
  );

  MarkerIndex marker_index = observed_scene.addMarker();
  userChangesVariableValue(variable_index, 1, tester);

  {
    TreePath marker_position_x_path =
      tree_paths.marker(marker_index).position.x;

    userChangesTreeItemExpression(marker_position_x_path, "var1", tester);
    assert(tree_widget.item(marker_position_x_path).maybe_numeric_value == 1);
  }

  observed_scene.removeVariable(variable_index);

  // Removing the variable invalidates the expression which doesn't change
  // the channel value.
  {
    TreePath marker_position_x_path =
      tree_paths.marker(marker_index).position.x;

    assert(tree_widget.item(marker_position_x_path).maybe_numeric_value == 1);
  }
}


namespace {
enum class BodyChannelType {
  translation_x,
  rotation_x
};
}


static TreePath
bodyChannelPath(
  BodyChannelType body_attribute,
  BodyIndex body_index,
  const TreePaths &tree_paths
)
{
  switch (body_attribute) {
    case BodyChannelType::translation_x:
      return tree_paths.body(body_index).translation.x.path;
    case BodyChannelType::rotation_x:
      return tree_paths.body(body_index).rotation.x.path;
  }

  assert(false); // shouldn't happen
}


static Expression
bodyChannelExpression(
  BodyChannelType body_attribute,
  BodyIndex body_index,
  const SceneState &scene_state
)
{
  switch (body_attribute) {
    case BodyChannelType::translation_x:
      return scene_state.body(body_index).expressions.translation.x;
    case BodyChannelType::rotation_x:
      return scene_state.body(body_index).expressions.rotation.x;
  }

  assert(false); // shouldn't happen
}


static void
testUsingAVariableInAnExpression(
  BodyChannelType body_attribute,
  bool with_good_expression
)
{
  Tester tester;
  ObservedScene &observed_scene = tester.observed_scene;
  SceneState &scene_state = observed_scene.scene_state;
  VariableIndex variable_index = observed_scene.addVariable();
  VariableName var_name = scene_state.variables[variable_index].name;
  TreePaths &tree_paths = observed_scene.tree_paths;
  BodyIndex body_index = observed_scene.addBody();

  TreePath variable_value_path =
    tree_paths.variables[variable_index].valuePath();

  const TreePath channel_path =
    bodyChannelPath(body_attribute, body_index, tree_paths);

  userChangesVariableValue(variable_index, 4, tester);
  Expression expr = (with_good_expression) ? var_name : "blah";
  userChangesTreeItemExpression(channel_path, expr, tester);

  if (with_good_expression) {
    assert(tester.tree_widget.item(channel_path).maybe_numeric_value == 4);
  }

  Expression channel_expression =
    bodyChannelExpression(body_attribute, body_index, scene_state);

  assert(channel_expression == expr);
  observed_scene.handleTreeNumericValueChanged(variable_value_path, 5);

  NumericValue channel_value =
    *tester.tree_widget.item(channel_path).maybe_numeric_value;

  if (with_good_expression) {
    assert(channel_value == 5);
  }
  else {
    assert(channel_value == 0);
  }
}


static void testUsingAVariable(BodyChannelType body_attribute)
{
  testUsingAVariableInAnExpression(
    body_attribute,
    /*with_good_expression*/true
  );
}


static void testUsingABadExpression()
{
  testUsingAVariableInAnExpression(
    BodyChannelType::rotation_x,
    /*with_good_expression*/false
  );
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


namespace {
struct BodySolveFlag {
  virtual bool
    &state(SceneState::TransformSolveFlags &transform_solve_flags) const = 0;

  virtual const TreePath &path(const TreePaths::Body &body_paths) const = 0;
};
}


namespace {
struct BodyTranslationXSolveFlag : BodySolveFlag {
  bool
  &state(SceneState::TransformSolveFlags &transform_solve_flags) const override
  {
    return transform_solve_flags.translation.x;
  }

  const TreePath &path(const TreePaths::Body &body_paths) const override
  {
    return body_paths.translation.x.solve_path;
  }
};
}


namespace {
struct BodyRotationXSolveFlag : BodySolveFlag {
  bool &
  state(SceneState::TransformSolveFlags &transform_solve_flags) const override
  {
    return transform_solve_flags.rotation.x;
  }

  const TreePath &path(const TreePaths::Body &body_paths) const override
  {
    return body_paths.rotation.x.solve_path;
  }
};
}


namespace {
struct BodyScaleSolveFlag : BodySolveFlag {
  bool &
  state(SceneState::TransformSolveFlags &transform_solve_flags) const override
  {
    return transform_solve_flags.scale;
  }

  const TreePath &path(const TreePaths::Body &body_paths) const override
  {
    return body_paths.scale.solve_path;
  }
};
}


static void testChangingBodySolveFlag(const BodySolveFlag &solve_flag)
{
  Tester tester;
  SceneState initial_state;
  BodyIndex body_index = initial_state.createBody();
  ObservedScene &observed_scene = tester.observed_scene;
  TreePaths &tree_paths = observed_scene.tree_paths;
  bool old_solve = true;
  solve_flag.state(initial_state.body(body_index).solve_flags) = old_solve;
  observed_scene.replaceSceneStateWith(initial_state);

  SceneState &scene_state = observed_scene.scene_state;

  const TreePath &solve_flag_path =
    solve_flag.path(tree_paths.body(body_index));

  FakeTreeItem::ValueString solve_value_string =
    tester.tree_widget.item(solve_flag_path).value_string;

  assert(solve_value_string == "value=1");
  observed_scene.handleTreeBoolValueChanged(solve_flag_path, !old_solve);
  bool new_solve = solve_flag.state(scene_state.body(body_index).solve_flags);
  assert(old_solve != new_solve);
}


static void testChangingBodyTranslationXSolveFlag()
{
  testChangingBodySolveFlag(BodyTranslationXSolveFlag());
}


static void testChangingBodyRotationXSolveFlag()
{
  testChangingBodySolveFlag(BodyRotationXSolveFlag());
}


static void testChangingBodyScaleSolveFlag()
{
  testChangingBodySolveFlag(BodyScaleSolveFlag());
}


static bool itemValueIsOn(const FakeTreeItem &item)
{
  FakeTreeItem::ValueString on_value_string =
    FakeTreeWidget::boolValueText(true);

  return (item.value_string == on_value_string);
}


namespace {
struct BodyElement {
  virtual const TreePath &path(const TreePaths::Body &) const = 0;
  virtual const BodySolveFlag &solveFlag() const = 0;
};
}


namespace {
struct BodyTranslationX : BodyElement {
  BodyTranslationXSolveFlag solve_flag;

  const TreePath &path(const TreePaths::Body &body_paths) const override
  {
    return body_paths.translation.x.path;
  }

  const BodySolveFlag &solveFlag() const override
  {
    return solve_flag;
  }
};
}


namespace {
struct BodyRotationX : BodyElement {
  BodyRotationXSolveFlag solve_flag;

  const TreePath &path(const TreePaths::Body &body_paths) const override
  {
    return body_paths.rotation.x.path;
  }

  const BodySolveFlag &solveFlag() const override
  {
    return solve_flag;
  }
};
}


namespace {
struct BodyScale : BodyElement {
  BodyScaleSolveFlag solve_flag;

  const TreePath &path(const TreePaths::Body &body_paths) const override
  {
    return body_paths.scale.path;
  }

  const BodySolveFlag &solveFlag() const override
  {
    return solve_flag;
  }
};
}


static void testChangingSolvableBodyValue(const BodyElement &element)
{
  const BodySolveFlag &solve_flag = element.solveFlag();

  SceneState initial_state;
  BodyIndex body_index = initial_state.createBody();
  solve_flag.state(initial_state.body(body_index).solve_flags) = true;
  Tester tester;
  ObservedScene &observed_scene = tester.observed_scene;
  observed_scene.replaceSceneStateWith(initial_state);
  TreePaths &tree_paths = observed_scene.tree_paths;
  const TreePaths::Body &body_paths = tree_paths.body(body_index);
  const FakeTreeWidget &tree_widget = tester.tree_widget;

  const TreePath &solve_flag_path = solve_flag.path(body_paths);
  const FakeTreeItem &solve_item = tree_widget.item(solve_flag_path);

  assert(itemValueIsOn(solve_item));

  observed_scene.handleTreeNumericValueChanged(
    element.path(body_paths), 2
  );

  assert(!itemValueIsOn(solve_item));
}


static void testChangingBodyTranslationXInTree()
{
  testChangingSolvableBodyValue(BodyTranslationX());
}


static void testChangingBodyRotationXInTree()
{
  testChangingSolvableBodyValue(BodyRotationX());
}


static void testChangingBodyScaleInTree()
{
  testChangingSolvableBodyValue(BodyScale());
}


static void
assertBoxIsUnscaled(
  BodyIndex body_index,
  BoxIndex box_index,
  const SceneState &scene_state
)
{
  const SceneState::XYZ &box_scale_state =
    scene_state.body(body_index).boxes[box_index].scale;

  assertNear(box_scale_state.x, 1, 0);
  assertNear(box_scale_state.y, 1, 0);
  assertNear(box_scale_state.z, 1, 0);
}


static void testMovingAMarkerInTheSceneThatAffectsScale()
{
  Tester tester;
  FakeScene &scene = tester.scene;
  ObservedScene &observed_scene = tester.observed_scene;
  SceneState initial_state;
  SceneHandles &scene_handles = observed_scene.scene_handles;
  SceneState &scene_state = observed_scene.scene_state;

  // Create a body
  BodyIndex body_index = initial_state.createBody();
  BoxIndex box_index = initial_state.body(body_index).createBox();

  // Put a local marker on the body at (1,0,0)
  MarkerIndex local_marker_index = initial_state.createMarker(body_index);
  initial_state.marker(local_marker_index).position = {1,0,0};

  // Put a global marker at (1,0,0)
  MarkerIndex global_marker_index = initial_state.createMarker();
  initial_state.marker(global_marker_index).position = {1,0,0};

  // Make the body scale be solved.
  initial_state.body(body_index).solve_flags.scale = true;

  observed_scene.replaceSceneStateWith(initial_state);

  Scene::TransformHandle global_marker_handle =
    scene_handles.marker(global_marker_index).transformHandle();

  Scene::TransformHandle local_marker_handle =
    scene_handles.marker(local_marker_index).transformHandle();

  assert(scene.translation(global_marker_handle) == Vec3(1,0,0));

  // Move the global marker in the scene.
  scene.setTranslation(global_marker_handle, {2,0,0});
  observed_scene.updateSceneStateFromSceneObjects();
  scene_state.body(body_index).transform.scale = 2;
  observed_scene.handleSceneStateChanged();

  // Check that the body was scaled but not the box.
  assert(scene_state.marker(global_marker_index).position.x == 2);
  assert(scene.translation(local_marker_handle) == Vec3(2,0,0));
  assertNear(scene_state.body(body_index).transform.scale, 2, 0);
  assertBoxIsUnscaled(body_index, box_index, scene_state);

  // Move it again.
  scene.setTranslation(global_marker_handle, {3,0,0});
  observed_scene.updateSceneStateFromSceneObjects();
  scene_state.body(body_index).transform.scale = 3;
  observed_scene.handleSceneStateChanged();

  // Check that the body was scaled but not the box.
  assert(scene_state.marker(global_marker_index).position.x == 3);
  assert(scene.translation(local_marker_handle) == Vec3(3,0,0));
  assertNear(scene_state.body(body_index).transform.scale, 3, 0);
  assertBoxIsUnscaled(body_index, box_index, scene_state);
}


static void
expectAllSolveFlagsAreOn(
  const TreePaths::XYZChannels &xyz_paths,
  const FakeTreeWidget &tree_widget
)
{
  const TreePath &x_solve_path = xyz_paths.x.solve_path;
  const TreePath &y_solve_path = xyz_paths.y.solve_path;
  const TreePath &z_solve_path = xyz_paths.z.solve_path;
  const FakeTreeItem &x_solve_item = tree_widget.item(x_solve_path);
  const FakeTreeItem &y_solve_item = tree_widget.item(y_solve_path);
  const FakeTreeItem &z_solve_item = tree_widget.item(z_solve_path);
  assert(itemValueIsOn(x_solve_item));
  assert(itemValueIsOn(y_solve_item));
  assert(itemValueIsOn(z_solve_item));
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


static void testSettingBoxScaleExpression()
{
  SceneState initial_state;
  BodyIndex body_index = initial_state.createBody();
  BoxIndex box_index = initial_state.body(body_index).createBox();

  Tester tester;
  ObservedScene &observed_scene = tester.observed_scene;
  observed_scene.replaceSceneStateWith(initial_state);
  const SceneState &scene_state = observed_scene.scene_state;
  const TreePaths &tree_paths = observed_scene.tree_paths;

  TreePath box_scale_x_path =
    tree_paths.body(body_index).boxes[box_index].scale.x;

  userChangesTreeItemExpression(box_scale_x_path, "5", tester);

  NumericValue value =
    scene_state.body(body_index).boxes[box_index].scale.x;

  assert(value == 5);
}


static void testSettingMarkerPositionInTree()
{
  SceneState initial_state;
  BodyIndex body_index = initial_state.createBody();
  MarkerIndex marker_index = initial_state.createMarker(body_index);

  Tester tester;
  ObservedScene &observed_scene = tester.observed_scene;
  observed_scene.replaceSceneStateWith(initial_state);
  TreePaths &tree_paths = observed_scene.tree_paths;
  SceneState &scene_state = observed_scene.scene_state;

  const TreePath &marker_position_x_path =
    tree_paths.marker(marker_index).position.x;

  observed_scene.handleTreeNumericValueChanged(marker_position_x_path, 5);
  assert(scene_state.marker(marker_index).position.x == 5);
}


static void testSettingChildBodyTransformValue()
{
  SceneState initial_state;
  BodyIndex parent_body_index = initial_state.createBody();
  BodyIndex child_body_index = initial_state.createBody(parent_body_index);

  Tester tester;
  ObservedScene &observed_scene = tester.observed_scene;
  observed_scene.replaceSceneStateWith(initial_state);
  TreePaths &tree_paths = observed_scene.tree_paths;
  SceneState &scene_state = observed_scene.scene_state;

  TreePath child_body_translation_x_path =
    tree_paths.body(child_body_index).translation.x.path;

  observed_scene.handleTreeNumericValueChanged(child_body_translation_x_path, 5);

  assert(scene_state.body(child_body_index).transform.translation.x == 5);
}


static void testHandleSceneStateChanged()
{
  Tester tester;
  ObservedScene &observed_scene = tester.observed_scene;
  SceneState &scene_state = observed_scene.scene_state;
  TreePaths &tree_paths = observed_scene.tree_paths;
  FakeTreeWidget &tree_widget = tester.tree_widget;

  // Add a body
  BodyIndex body_index = observed_scene.addBody();

  // Change the body scale in the state.
  scene_state.body(body_index).transform.scale = 2.5;

  // call handleSceneStateChanged()
  observed_scene.handleSceneStateChanged();

  // check that the scale value is updated in the tree.
  const TreePath &scale_path = tree_paths.body(body_index).scale.path;
  assert(tree_widget.item(scale_path).maybe_numeric_value == 2.5);
}


int main()
{
  testTransferringABody1();
  testTransferringABody2();
  testTransferringAMarker();
  testHandleSceneStateChanged();
  testDuplicateBody();
  testDuplicateBodyWhenTheBodyHasExpressions();
  testDuplicateBodyWithDistanceErrors();
  testUserSelectingABodyInTheTree();
  testSelectingSceneInTheTree();
  testSelectingMarkerInTheScene();
  testCutAndPaste();
  testAddingADistanceErrorToABody();
  testReplacingSceneState();
  testAddingAndRemovingAVariable();
  testChangingMarkerName();
  testDuplicatingAMarkerWithDistanceError();
  testChangingBodyTranslationXSolveFlag();
  testChangingBodyRotationXSolveFlag();
  testChangingBodyScaleSolveFlag();
  testChangingBodyTranslationXInTree();
  testChangingBodyRotationXInTree();
  testChangingBodyScaleInTree();
  testMovingAMarkerInTheSceneThatAffectsScale();
  testSetSolveFlags();
  testUsingAVariable(BodyChannelType::translation_x);
  testUsingAVariable(BodyChannelType::rotation_x);
  testUsingABadExpression();
  testSettingBoxScaleExpression();
  testSettingMarkerPositionInTree();
  testSettingChildBodyTransformValue();
}
