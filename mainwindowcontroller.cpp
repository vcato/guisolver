#include "mainwindowcontroller.hpp"

#include "vectorio.hpp"
#include "sceneerror.hpp"
#include "scenesolver.hpp"
#include "treevalues.hpp"
#include "sceneobjects.hpp"
#include "matchconst.hpp"
#include "scenestatetaggedvalue.hpp"
#include "observedscene.hpp"
#include "scenestateio.hpp"
#include "parsedouble.hpp"
#include "evaluateexpression.hpp"

using View = MainWindowView;
using std::cerr;
using std::string;
using TransformHandle = Scene::TransformHandle;
using GeometryHandle = Scene::GeometryHandle;
using TreeItemDescription = ObservedScene::TreeItemDescription;
using ManipulatorType = Scene::ManipulatorType;


template <typename XYZSolveFlags, typename F>
static void
forEachSolveFlagInXYZ(
  XYZSolveFlags &solve_flags,
  const F &f,
  const SceneState::XYZSolveFlags &visit
)
{
  if (visit.x) f(solve_flags.x);
  if (visit.y) f(solve_flags.y);
  if (visit.z) f(solve_flags.z);
}


template <typename TransformSolveFlags, typename F>
static void
forEachSolveFlagInTransform(
  TransformSolveFlags &solve_flags,
  const F &f,
  const SceneState::TransformSolveFlags &visit
)
{
  forEachSolveFlagInXYZ(solve_flags.translation, f, visit.translation);
  forEachSolveFlagInXYZ(solve_flags.rotation, f, visit.rotation);
}


template <typename F>
static bool anyFlagsAreSet(const F &for_each_function)
{
  bool any_are_set = false;
  for_each_function([&](bool arg){ if (arg) any_are_set = true; });
  return any_are_set;
}


template <typename SceneState, typename F>
static void
  forEachSolveFlagAffectingBody(
    BodyIndex body_index,
    SceneState &state,
    const F &f,
    const typename SceneState::TransformSolveFlags &visit
  )
{
  forEachSolveFlagInTransform(state.body(body_index).solve_flags, f, visit);

  Optional<BodyIndex> maybe_parent_index =
    state.body(body_index).maybe_parent_index;

  if (!maybe_parent_index) {
    return;
  }

  return forEachSolveFlagAffectingBody(*maybe_parent_index, state, f, visit);
}


template <typename SceneState, typename F>
static void
forEachSolveFlagAffectingHandle(
  TransformHandle handle,
  const SceneHandles &scene_handles,
  SceneState &state,
  Optional<ManipulatorType> maybe_manipulator_type,
  const F &f
)
{
  // If the handle is for a body that is a child of the box, then it
  // is going to be affected by the solve.
  for (auto i : indicesOf(state.bodies())) {
    if (handle == scene_handles.body(i).transformHandle()) {
      typename SceneState::TransformSolveFlags visit;
      setAll(visit, true);

      if (maybe_manipulator_type == ManipulatorType::translate) {
        // If we're using a translation manipulator, the rotations don't
        // affect it.
        setAll(visit.rotation, false);
      }

      forEachSolveFlagAffectingBody(i, state, f, visit);
    }
  }

  for (auto i : indicesOf(state.markers())) {
    if (handle == scene_handles.marker(i).transformHandle()) {
      typename SceneState::TransformSolveFlags visit;
      setAll(visit, true);
      Optional<BodyIndex> maybe_body_index = state.marker(i).maybe_body_index;

      if (maybe_body_index) {
        forEachSolveFlagAffectingBody(*maybe_body_index, state, f, visit);
      }
    }
  }
}


static void
  setSceneStateEnumerationIndex(
    SceneState &scene_state,
    const TreePath &path,
    int value,
    const TreePaths &tree_paths
  )
{
  for (auto i : indicesOf(tree_paths.distance_errors)) {
    const TreePaths::DistanceError &distance_error_paths =
      tree_paths.distance_errors[i];

    auto &state_distance_error = scene_state.distance_errors[i];

    if (path == distance_error_paths.start) {
      state_distance_error.optional_start_marker_index =
        markerIndexFromEnumerationValue(value);
    }

    if (path == distance_error_paths.end) {
      state_distance_error.optional_end_marker_index =
        markerIndexFromEnumerationValue(value);
    }
  }
}


static Optional<NumericValue> maybeValueFromText(const string &arg)
{
  if (arg.length() == 0) {
    return {};
  }

  if (arg[0] != '=') {
    return parseDouble(arg);
  }

  string expr = arg.substr(1);
  return evaluateExpression(expr, cerr);
}


struct MainWindowController::Impl {
  struct Data {
    View &view;
    ObservedScene observed_scene;
    Clipboard clipboard;

    Data(View &, Scene &, TreeWidget &);
  };

  Data data_member;

  Impl(View &view, Scene &scene, TreeWidget &tree_widget)
  : data_member(view, scene, tree_widget)
  {
  }

  static Impl &impl(MainWindowController &controller)
  {
    assert(controller.impl_ptr);
    return *controller.impl_ptr;
  }

  static Data &data(MainWindowController &controller)
  {
    return impl(controller).data_member;
  }

  static ObservedScene &observedScene(MainWindowController &controller)
  {
    return data(controller).observed_scene;
  }

  static void handleSceneChanging(MainWindowController &);
  static void handleSceneChanged(MainWindowController &);

  static void
    handleTreeNumericValueChanged(
      MainWindowController &,
      const TreePath &,
      NumericValue
    );

  static Optional<NumericValue>
  evaluateInput(const string &text, const TreePath &)
  {
    return maybeValueFromText(text);
  }

  static void
    handleTreeStringValueChanged(
      MainWindowController &,
      const TreePath &,
      const StringValue &
    );

  static void
    handleTreeEnumerationIndexChanged(
      MainWindowController &controller,
      const TreePath &path,
      int value
    );

  static TreeWidget::MenuItems
    contextMenuItemsForPath(
      MainWindowController &controller,
      const TreePath &path
    );

  static void
    handleSolveToggleChange(MainWindowController &, const TreePath &);

  static void
    addDistanceErrorPressed(
      MainWindowController &controller,
      Optional<BodyIndex>
    );

  static void addVariablePressed(MainWindowController &controller);

  static void
    pasteGlobalPressed(MainWindowController &controller, const TreePath &path);

  static void
    solveAllPressed(
      MainWindowController &controller,
      const TreeItemDescription &,
      bool state
    );

  static void addMarkerPressed(MainWindowController &, const TreePath &);
  static void addBodyPressed(MainWindowController &, const TreePath &);
  static void addBoxPressed(MainWindowController &, BodyIndex);
  static void addLinePressed(MainWindowController &, BodyIndex);
  static void removeBodyPressed(MainWindowController &, BodyIndex);
  static void duplicateBodyPressed(MainWindowController &, BodyIndex);

  static void
    duplicateBodyWithDistanceErrorsPressed(MainWindowController &, BodyIndex);

  static void
    duplicateMarkerWithDistanceErrorPressed(
      MainWindowController &, MarkerIndex
    );

  static void
    removeDistanceErrorPressed(
      MainWindowController &,
      int distance_error_index
    );

  static void
    removeMarkerPressed(
      MainWindowController &,
      MarkerIndex
    );

  static void
    removeBoxPressed(
      MainWindowController &,
      BodyIndex,
      BoxIndex
    );

  static void
    duplicateMarkerPressed(
      MainWindowController &,
      MarkerIndex
    );

  static void cutBodyPressed(MainWindowController &, const TreePath &);
  static void cutMarkerPressed(MainWindowController &, MarkerIndex);
};


static Optional<TransformHandle> selectedObjectTransform(const Scene &scene)
{
  Optional<GeometryHandle>
    maybe_selected_geometry = scene.selectedGeometry();

  if (!maybe_selected_geometry) {
    return scene.selectedTransform();
  }

  return scene.parentTransform(*maybe_selected_geometry);
}


void
  MainWindowController::Impl::handleSceneChanging(
    MainWindowController &controller
  )
{
  ObservedScene &observed_scene = Impl::observedScene(controller);
  // The mouse button is down.  The scene is being changed, but we don't
  // consider this change complete.

  Scene &scene = observed_scene.scene;
  SceneHandles &scene_handles = observed_scene.scene_handles;
  SceneState &state = observed_scene.scene_state;

  Optional<TransformHandle> maybe_transform_handle =
    selectedObjectTransform(scene);

  assert(maybe_transform_handle);
    // How could the scene be changing if nothing was selected?

  TransformHandle transform_handle = *maybe_transform_handle;

  updateSceneStateFromSceneObjects(state, scene, scene_handles);

  vector<bool> old_flags;

  // Disable any transforms that would move the handle and remember the
  // old state.

  Optional<ManipulatorType> maybe_manipulator_type =
    observed_scene.properManpiulatorForSelectedObject();

  forEachSolveFlagAffectingHandle(
    transform_handle, scene_handles, state,
    maybe_manipulator_type,
    [&](bool &arg){
      old_flags.push_back(arg);
      arg = false;
    }
  );

  solveScene(state);

  // Restore the old solve states.
  {
    vector<bool>::const_iterator iter = old_flags.begin();

    forEachSolveFlagAffectingHandle(
      transform_handle, scene_handles, state,
      maybe_manipulator_type,
      [&](bool &arg){
        arg = *iter++;
      }
    );

    assert(iter == old_flags.end());
  }

  observed_scene.handleSceneStateChanged();
}


void
  MainWindowController::Impl::handleSceneChanged(
    MainWindowController &controller
  )
{
  ObservedScene &observed_scene = observedScene(controller);
  SceneHandles &scene_handles = observed_scene.scene_handles;
  Scene &scene = observed_scene.scene;
  SceneState &state = observed_scene.scene_state;

  updateSceneStateFromSceneObjects(state, scene, scene_handles);
  solveScene(state);
  observed_scene.handleSceneStateChanged();
}


template <typename XYZSolveFlags>
static MatchConst_t<bool, XYZSolveFlags> *
  xyzSolveStatePtr(
    XYZSolveFlags &xyz_solve_flags,
    const TreePath &path,
    const TreePaths::XYZ &xyz_paths
  )
{
  if (path == xyz_paths.x) return &xyz_solve_flags.x;
  if (path == xyz_paths.y) return &xyz_solve_flags.y;
  if (path == xyz_paths.z) return &xyz_solve_flags.z;
  return nullptr;
}


template <typename SceneState>
static MatchConst_t<bool, SceneState> *
  bodySolveStatePtr(
    SceneState &scene_state,
    const TreePath &path,
    const TreePaths &tree_paths,
    BodyIndex body_index
  )
{
  using Bool = MatchConst_t<bool, SceneState>;
  auto &body_state = scene_state.body(body_index);
  const TreePaths::Body &body_paths = tree_paths.body(body_index);
  {
    const TreePaths::XYZ &xyz_paths = body_paths.translation;
    auto &xyz_solve_flags = body_state.solve_flags.translation;
    Bool *result_ptr = xyzSolveStatePtr(xyz_solve_flags, path, xyz_paths);

    if (result_ptr) {
      return result_ptr;
    }
  }
  {
    const TreePaths::XYZ &xyz_paths = body_paths.rotation;
    auto &xyz_solve_flags = body_state.solve_flags.rotation;
    Bool *result_ptr = xyzSolveStatePtr(xyz_solve_flags, path, xyz_paths);

    if (result_ptr) {
      return result_ptr;
    }
  }

  return nullptr;
}


template <typename SceneState>
static auto*
  solveStatePtr(
    SceneState &scene_state,
    const TreePath &path,
    const TreePaths &tree_paths
  )
{
  for (auto body_index : indicesOf(scene_state.bodies())) {
    auto *solve_state_ptr =
      bodySolveStatePtr(scene_state, path, tree_paths, body_index);

    if (solve_state_ptr) {
      return solve_state_ptr;
    }
  }

  return
    decltype(bodySolveStatePtr(scene_state, path, tree_paths, 0))(nullptr);
}


void
MainWindowController::Impl::handleTreeNumericValueChanged(
  MainWindowController &controller,
  const TreePath &path,
  NumericValue value
)
{
  ObservedScene &observed_scene = observedScene(controller);
  const TreePaths &tree_paths = observed_scene.tree_paths;
  SceneState &state = observed_scene.scene_state;

  bool value_was_changed =
    setSceneStateNumericValue(state, path, value, tree_paths);

  if (value_was_changed) {
    {
      bool *solve_state_ptr = solveStatePtr(state, path, tree_paths);
      Optional<bool> maybe_old_state;

      // Turn off the solve state of the value that is being changed, so that
      // it doesn't give strange feedback to the user.

      if (solve_state_ptr) {
        maybe_old_state = *solve_state_ptr;
        *solve_state_ptr = false;
      }

      solveScene(state);

      if (solve_state_ptr) {
        *solve_state_ptr = *maybe_old_state;
      }
    }

    observed_scene.handleSceneStateChanged();
  }
  else {
    cerr << "Handling spin_box_item_value_changed_function\n";
    cerr << "  path: " << path << "\n";
    cerr << "  value: " << value << "\n";
  }
}


void
MainWindowController::Impl::handleTreeStringValueChanged(
  MainWindowController &controller,
  const TreePath &path,
  const StringValue &value
)
{
  ObservedScene &observed_scene = observedScene(controller);
  const TreePaths &tree_paths = observed_scene.tree_paths;
  SceneState &state = observed_scene.scene_state;
  TreeWidget &tree_widget = observed_scene.tree_widget;

  bool value_was_changed =
    setSceneStateStringValue(state, path, value, tree_paths);

  if (value_was_changed) {
    updateTreeValues(tree_widget, tree_paths, state);
    updateTreeDistanceErrorMarkerOptions(tree_widget, tree_paths, state);
  }
  else {
    cerr << "handleTreeStringValueChanged: no match\n";
  }
}


void
  MainWindowController::Impl::handleTreeEnumerationIndexChanged(
    MainWindowController &controller,
    const TreePath &path,
    int value
  )
{
  ObservedScene &observed_scene = observedScene(controller);
  const TreePaths &tree_paths = observed_scene.tree_paths;
  SceneState &scene_state = observed_scene.scene_state;

  setSceneStateEnumerationIndex(scene_state, path, value, tree_paths);
  solveScene(scene_state);
  observed_scene.handleSceneStateChanged();
}


void
  MainWindowController::Impl::addDistanceErrorPressed(
    MainWindowController &controller,
    Optional<BodyIndex> optional_body_index
  )
{
  ObservedScene &observed_scene = observedScene(controller);

  DistanceErrorIndex index =
    observed_scene.addDistanceError(
      /*maybe_start_marker_index*/{},
      /*maybe_end_marker_index*/{},
      optional_body_index
    );

  observed_scene.selectDistanceError(index);
}


void
  MainWindowController::Impl::addVariablePressed(
    MainWindowController &controller
  )
{
  ObservedScene &observed_scene = observedScene(controller);
  VariableIndex index = observed_scene.addVariable();
  observed_scene.selectVariable(index);
}


static BodyIndex
  bodyIndexFromTreePath(const TreePath &path, const TreePaths &tree_paths)
{
  for (auto i : indicesOf(tree_paths.bodies)) {
    if (tree_paths.body(i).path == path) {
      return i;
    }
  }

  assert(false);
}


static Optional<BodyIndex>
maybeBodyIndexFromTreePath(const TreePath &path, const TreePaths &tree_paths)
{
  if (tree_paths.path == path) {
    return {};
  }

  return bodyIndexFromTreePath(path, tree_paths);

}


void
  MainWindowController::Impl::addMarkerPressed(
    MainWindowController &controller,
    const TreePath &path
  )
{
  ObservedScene &observed_scene = observedScene(controller);
  TreePaths &tree_paths = observed_scene.tree_paths;

  Optional<BodyIndex> maybe_body_index =
    maybeBodyIndexFromTreePath(path, tree_paths);;

  MarkerIndex marker_index = observed_scene.addMarker(maybe_body_index);
  observed_scene.selectMarker(marker_index);
}


void
MainWindowController::Impl::pasteGlobalPressed(
  MainWindowController &controller,
  const TreePath &path
)
{
  ObservedScene &observed_scene = observedScene(controller);
  SceneState &scene_state = observed_scene.scene_state;

  Optional<BodyIndex> maybe_new_parent_body_index =
    maybeBodyIndexFromTreePath(path, observed_scene.tree_paths);

  if (observed_scene.clipboardContainsABody()) {
    BodyIndex new_body_index =
      observed_scene.pasteBodyGlobal(maybe_new_parent_body_index);

    observed_scene.selectBody(new_body_index);
    solveScene(scene_state);
    observed_scene.handleSceneStateChanged();
    return;
  }

  if (observed_scene.clipboardContainsAMarker()) {
    MarkerIndex new_marker_index =
      observed_scene.pasteMarkerGlobal(maybe_new_parent_body_index);

    observed_scene.selectMarker(new_marker_index);
    solveScene(scene_state);
    observed_scene.handleSceneStateChanged();
    return;
  }

  else {
    assert(false); // not implemented
  }
}


void
MainWindowController::Impl::solveAllPressed(
  MainWindowController &controller,
  const TreeItemDescription &item,
  bool state
)
{
  ObservedScene &observed_scene = observedScene(controller);
  SceneState &scene_state = observed_scene.scene_state;
  using ItemType = TreeItemDescription::Type;

  SceneState::TransformSolveFlags &solve_flags =
    scene_state.body(*item.maybe_body_index).solve_flags;

  if (item.type == ItemType::translation) {
    setAll(solve_flags.translation, state);
  }

  if (item.type == ItemType::rotation) {
    setAll(solve_flags.rotation, state);
  }

  solveScene(scene_state);
  observed_scene.handleSceneStateChanged();
}


void
MainWindowController::Impl::addBodyPressed(
  MainWindowController &controller,
  const TreePath &parent_path
)
{
  ObservedScene &observed_scene = observedScene(controller);
  TreePaths &tree_paths = observed_scene.tree_paths;

  Optional<BodyIndex> maybe_parent_body_index =
    maybeBodyIndexFromTreePath(parent_path, tree_paths);

  BodyIndex new_body_index = observed_scene.addBody(maybe_parent_body_index);
  observed_scene.selectBody(new_body_index);
}


void
MainWindowController::Impl::addBoxPressed(
  MainWindowController &controller,
  BodyIndex body_index
)
{
  ObservedScene &observed_scene = observedScene(controller);
  BoxIndex box_index = observed_scene.addBoxTo(body_index);
  observed_scene.selectBox(body_index, box_index);
}


void
MainWindowController::Impl::addLinePressed(
  MainWindowController &controller, BodyIndex body_index
)
{
  ObservedScene &observed_scene = observedScene(controller);
  LineIndex line_index = observed_scene.addLineTo(body_index);
  observed_scene.selectLine(body_index, line_index);
}


void
MainWindowController::Impl::removeBodyPressed(
  MainWindowController &controller, BodyIndex body_index
)
{
  observedScene(controller).removeBody(body_index);
}


void
MainWindowController::Impl::duplicateBodyPressed(
  MainWindowController &controller, BodyIndex body_index
)
{
  ObservedScene &observed_scene = observedScene(controller);
  BodyIndex new_body_index = observed_scene.duplicateBody(body_index);
  observed_scene.selectBody(new_body_index);
}


void
MainWindowController::Impl::duplicateBodyWithDistanceErrorsPressed(
  MainWindowController &controller, BodyIndex body_index
)
{
  ObservedScene &observed_scene = observedScene(controller);

  BodyIndex new_body_index =
    observed_scene.duplicateBodyWithDistanceErrors(body_index);

  observed_scene.selectBody(new_body_index);
}


void
MainWindowController::Impl::duplicateMarkerWithDistanceErrorPressed(
  MainWindowController &controller, MarkerIndex marker_index
)
{
  ObservedScene &observed_scene = observedScene(controller);

  BodyIndex new_marker_index =
    observed_scene.duplicateMarkerWithDistanceError(marker_index);

  observed_scene.selectMarker(new_marker_index);
}


void
  MainWindowController::Impl::removeDistanceErrorPressed(
    MainWindowController &controller,
    int distance_error_index
  )
{
  ObservedScene &observed_scene = observedScene(controller);
  SceneState &scene_state = observed_scene.scene_state;
  Scene &scene = observed_scene.scene;
  SceneHandles &scene_handles = observed_scene.scene_handles;
  TreePaths &tree_paths = observed_scene.tree_paths;
  TreeWidget &tree_widget = observed_scene.tree_widget;

  removeDistanceErrorFromTree(
    distance_error_index,
    { tree_widget, tree_paths }
  );

  removeDistanceErrorFromScene(
    scene,
    scene_handles.distance_errors,
    distance_error_index
  );

  scene_state.removeDistanceError(distance_error_index);
  solveScene(scene_state);
  observed_scene.handleSceneStateChanged();
}


void
  MainWindowController::Impl::removeMarkerPressed(
    MainWindowController &controller,
    MarkerIndex marker_index
  )
{
  ObservedScene &observed_scene = observedScene(controller);
  TreePaths &tree_paths = observed_scene.tree_paths;
  TreeWidget &tree_widget = observed_scene.tree_widget;
  SceneState &scene_state = observed_scene.scene_state;
  observed_scene.removeMarker( marker_index);
  updateTreeDistanceErrorMarkerOptions(tree_widget, tree_paths, scene_state);
}


void
  MainWindowController::Impl::duplicateMarkerPressed(
    MainWindowController &controller,
    MarkerIndex source_marker_index
  )
{
  ObservedScene &observed_scene = observedScene(controller);
  TreePaths &tree_paths = observed_scene.tree_paths;
  TreeWidget &tree_widget = observed_scene.tree_widget;
  SceneState &scene_state = observed_scene.scene_state;

  MarkerIndex new_marker_index =
    observed_scene.duplicateMarker(source_marker_index);

  updateTreeDistanceErrorMarkerOptions(tree_widget, tree_paths, scene_state);
  observed_scene.selectMarker(new_marker_index);
}


void
MainWindowController::Impl::cutMarkerPressed(
  MainWindowController &controller,
  MarkerIndex marker_index
)
{
  observedScene(controller).cutMarker(marker_index);
}


void
  MainWindowController::Impl::cutBodyPressed(
    MainWindowController &controller,
    const TreePath &path
  )
{
  ObservedScene &observed_scene = observedScene(controller);
  TreePaths &tree_paths = observed_scene.tree_paths;
  BodyIndex body_index = bodyIndexFromTreePath(path, tree_paths);
  observed_scene.cutBody(body_index);
}


template <typename T>
static void appendTo(vector<T> &v, const vector<T> &n)
{
  v.insert(v.end(), n.begin(), n.end());
}


static void flip(bool &arg)
{
  arg = !arg;
}


static bool
  solveState(
    const SceneState &scene_state,
    const TreePath &path,
    const TreePaths &tree_paths
  )
{
  const bool *solve_state_ptr = solveStatePtr(scene_state, path, tree_paths);

  if (!solve_state_ptr) {
    assert(false); // not implemented
  }

  return *solve_state_ptr;
}


static void
  flipSolveState(
    SceneState &scene_state,
    const TreePath &path,
    const TreePaths &tree_paths
  )
{
  bool *solve_state_ptr =
    const_cast<bool *>(solveStatePtr(scene_state, path, tree_paths));


  if (!solve_state_ptr) {
    assert(false); // not implemented
  }

  flip(*solve_state_ptr);
}


void
  MainWindowController::Impl::handleSolveToggleChange(
    MainWindowController &controller,
    const TreePath &path
  )
{
  ObservedScene &observed_scene = observedScene(controller);
  SceneState &state = observed_scene.scene_state;
  TreePaths &tree_paths = observed_scene.tree_paths;
  flipSolveState(state, path, tree_paths);
  solveScene(state);
  observed_scene.handleSceneStateChanged();
}


TreeWidget::MenuItems
  MainWindowController::Impl::contextMenuItemsForPath(
    MainWindowController &controller,
    const TreePath &path
  )
{
  ObservedScene &observed_scene = observedScene(controller);
  TreeWidget::MenuItems menu_items;
  const TreePaths &tree_paths = observed_scene.tree_paths;
  const SceneState &scene_state = observed_scene.scene_state;
  TreeItemDescription item = ObservedScene::describePath(path, tree_paths);
  using ItemType = TreeItemDescription::Type;
  const ItemType item_type = item.type;

  auto add_marker_function =
    [&controller,path]{
      Impl::addMarkerPressed(controller, path);
    };

  auto add_body_function =
    [&controller,path]{ Impl::addBodyPressed(controller, path); };

  auto paste_global_function =
    [&controller,path]{
      Impl::pasteGlobalPressed(controller, path);
    };

  if (item_type == ItemType::translation || item_type == ItemType::rotation) {
    auto solve_all_on_function =
      [&controller,item]{
        Impl::solveAllPressed(controller, item, true);
      };

    auto solve_all_off_function =
      [&controller,item]{
        Impl::solveAllPressed(controller, item, false);
      };

    appendTo(menu_items,{
      {"Solve All On", solve_all_on_function },
      {"Solve All Off", solve_all_off_function },
    });
  }

  if (item_type == ItemType::scene) {
    auto add_distance_error_function =
      [&controller,path]{
        Impl::addDistanceErrorPressed(controller, /*optional_body_index*/{});
      };

    auto add_variable_function =
      [&controller, path]{
        Impl::addVariablePressed(controller);
      };

    appendTo(menu_items,{
      {"Add Distance Error", add_distance_error_function },
      {"Add Marker", add_marker_function },
      {"Add Body", add_body_function },
      {"Add Variable", add_variable_function },
    });

    if (observed_scene.canPasteTo({})) {
      appendTo(menu_items,{
        {"Paste Preserving Global", paste_global_function}
      });
    }
  }

  if (item_type == ItemType::body) {
    BodyIndex body_index = bodyIndexFromTreePath(path, tree_paths);

    auto cut_body_function =
      [&controller,path]{
        Impl::cutBodyPressed(controller, path);
      };

    auto remove_body_function =
      [&controller,body_index]{
        Impl::removeBodyPressed(controller, body_index);
      };

    auto duplicate_body_function =
      [&controller,body_index]{
        Impl::duplicateBodyPressed(controller, body_index);
      };

    auto duplicate_body_with_distance_errors_function =
      [&controller,body_index]{
        Impl::duplicateBodyWithDistanceErrorsPressed(controller, body_index);
      };

    auto add_box_function =
      [&controller,body_index]{
        Impl::addBoxPressed(controller, body_index);
      };

    auto add_line_function =
      [&controller,body_index]{
        Impl::addLinePressed(controller, body_index);
      };

    auto add_distance_error_function =
      [&controller,body_index]{
        Impl::addDistanceErrorPressed(controller, body_index);
      };

    appendTo(menu_items,{
      {"Add Marker", add_marker_function},
      {"Add Body", add_body_function},
      {"Add Box", add_box_function},
      {"Add Line", add_line_function},
      {"Add Distance Error", add_distance_error_function},
      {"Cut", cut_body_function },
      {"Remove", remove_body_function },
      {"Duplicate", duplicate_body_function },
      {"Duplicate With Distance Errors",
        duplicate_body_with_distance_errors_function },
    });

    if (observed_scene.canPasteTo(body_index)) {
      appendTo(menu_items,{
        {"Paste Preserving Global", paste_global_function}
      });
    }
  }

  if (item_type == ItemType::box) {
    BodyIndex body_index = *item.maybe_body_index;
    size_t box_index = *item.maybe_box_index;

    auto remove_box_function =
      [&observed_scene, body_index, box_index]{
        observed_scene.removeBox(body_index, box_index);
      };

    appendTo(menu_items,{
      {"Remove", remove_box_function}
    });
  }

  if (item_type == ItemType::line) {
    BodyIndex body_index = *item.maybe_body_index;
    size_t line_index = *item.maybe_line_index;

    auto remove_line_function =
      [&observed_scene, body_index, line_index]{
        observed_scene.removeLine(body_index, line_index);
      };

    appendTo(menu_items,{
      {"Remove", remove_line_function}
    });
  }

  if (item_type == ItemType::marker) {
    auto index = *item.maybe_marker_index;

    auto remove_marker_function = [&controller,index]{
      Impl::removeMarkerPressed(controller, index);
    };

    auto duplicate_marker_function = [&controller,index]{
      Impl::duplicateMarkerPressed(controller, index);
    };

    auto duplicate_marker_with_distance_error_function =
      [&controller,index]{
        Impl::duplicateMarkerWithDistanceErrorPressed(controller, index);
      };

    auto cut_marker_function = [&controller,index]{
      Impl::cutMarkerPressed(controller, index);
    };

    appendTo(menu_items,{
      {"Remove", remove_marker_function},
      {"Duplicate", duplicate_marker_function},
      {"Cut", cut_marker_function},
      {"Duplicate With Distance Error",
        duplicate_marker_with_distance_error_function },
    });
  }

  if (item_type == ItemType::distance_error) {
    auto index = *item.maybe_distance_error_index;

    auto remove_distance_error_function =
      [&controller,index]{
        Impl::removeDistanceErrorPressed(controller, index);
      };

    appendTo(menu_items,{
      {"Remove", remove_distance_error_function}
    });
  }

  if (solveStatePtr(scene_state, path, tree_paths)) {
    auto solve_function =
      [&controller,path](){
        Impl::handleSolveToggleChange(controller,path);
      };

    bool checked_state = solveState(scene_state, path, tree_paths);

    appendTo(menu_items,{
      {"Solve", solve_function, checked_state}
    });
  }

  return menu_items;
}


MainWindowController::Impl::Data::Data(
  View &view_arg,
  Scene &scene_arg,
  TreeWidget &tree_widget_arg
)
: view(view_arg),
  observed_scene(
    scene_arg,
    tree_widget_arg,
    [](SceneState &state){
      updateErrorsInState(state);
    }
  )
{
}


MainWindowController::MainWindowController(View &view)
: impl_ptr(new Impl(view, view.scene(), view.treeWidget()))
{
  TreeWidget &tree_widget = view.treeWidget();
  Scene &scene = view.scene();
  ObservedScene &observed_scene = Impl::observedScene(*this);
  scene.changed_callback = [&]{ Impl::handleSceneChanged(*this); };
  scene.changing_callback = [&]{ Impl::handleSceneChanging(*this); };

  scene.selection_changed_callback =
    [&observed_scene]{ observed_scene.handleSceneSelectionChanged(); };

  tree_widget.spin_box_item_value_changed_callback =
    [this](const TreePath &path, NumericValue value){
      Impl::handleTreeNumericValueChanged(*this, path, value);
    };

  tree_widget.evaluate_function =
    [](const TreePath &path, const string &text){
      return Impl::evaluateInput(text, path);
    };

  tree_widget.enumeration_item_index_changed_callback =
    [this](const TreePath &path, int index){
      Impl::handleTreeEnumerationIndexChanged(*this, path, index);
    };

  tree_widget.selection_changed_callback =
    [&observed_scene](){
      observed_scene.handleTreeSelectionChanged();
    };

  tree_widget.context_menu_items_callback =
    [this](const TreePath &path){
      return Impl::contextMenuItemsForPath(*this, path);
    };

  tree_widget.line_edit_item_value_changed_callback =
    [this](const TreePath &path, const StringValue &value){
      Impl::handleTreeStringValueChanged(*this, path, value);
    };
}


MainWindowController::~MainWindowController() = default;


void MainWindowController::replaceSceneStateWith(const SceneState &new_state)
{
  ObservedScene &observed_scene = Impl::observedScene(*this);
  SceneState solved_new_state = new_state;
  solveScene(solved_new_state);
  observed_scene.replaceSceneStateWith(solved_new_state);
}


void MainWindowController::newPressed()
{
  replaceSceneStateWith(SceneState());
}


void MainWindowController::savePressed()
{
  Optional<string> maybe_path = Impl::data(*this).view.askForSavePath();

  if (!maybe_path) {
    // Cancelled
  }
  else {
    saveScene(Impl::observedScene(*this).scene_state, *maybe_path);
  }
}


void MainWindowController::openPressed()
{
  Optional<string> maybe_path = Impl::data(*this).view.askForOpenPath();

  if (!maybe_path) {
    // Cancelled
  }
  else {
    SceneState new_scene_state;
    loadScene(new_scene_state, *maybe_path);
    replaceSceneStateWith(new_scene_state);
  }
}
