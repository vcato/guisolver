#include "mainwindowcontroller.hpp"

#include "vectorio.hpp"
#include "sceneerror.hpp"
#include "scenesolver.hpp"
#include "treevalues.hpp"
#include "sceneobjects.hpp"
#include "matchconst.hpp"
#include "scenestatetaggedvalue.hpp"
#include "observedscene.hpp"

using std::cerr;
using TransformHandle = Scene::TransformHandle;


static bool
  hasAncestor(
    BodyIndex body_index,
    BodyIndex ancestor_body_index,
    const SceneState &state
  )
{
  if (body_index == ancestor_body_index) {
    return true;
  }

  Optional<BodyIndex> maybe_parent_body_index =
    state.body(body_index).maybe_parent_index;

  if (!maybe_parent_body_index) {
    return false;
  }

  return hasAncestor(*maybe_parent_body_index, ancestor_body_index, state);
}


template <typename XYZSolveFlags, typename F>
static void forEachSolveFlagInXYZ(XYZSolveFlags &solve_flags, const F &f)
{
  f(solve_flags.x);
  f(solve_flags.y);
  f(solve_flags.z);
}


template <typename TransformSolveFlags, typename F>
static void
forEachSolveFlagInTransform(TransformSolveFlags &solve_flags, const F &f)
{
  forEachSolveFlagInXYZ(solve_flags.translation, f);
  forEachSolveFlagInXYZ(solve_flags.rotation, f);
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
    const F &f
  )
{
  forEachSolveFlagInTransform(state.body(body_index).solve_flags, f);

  Optional<BodyIndex> maybe_parent_index =
    state.body(body_index).maybe_parent_index;

  if (!maybe_parent_index) {
    return;
  }

  return forEachSolveFlagAffectingBody(*maybe_parent_index, state, f);
}


template <typename SceneState, typename F>
static void
forEachSolveFlagAffectingHandle(
  TransformHandle handle,
  const SceneHandles &scene_handles,
  SceneState &state,
  const F &f
)
{
  // If the handle is for a body that is a child of the box, then it
  // is going to be affected by the solve.
  for (auto i : indicesOf(state.bodies())) {
    if (handle == scene_handles.bodies[i]) {
      forEachSolveFlagAffectingBody(i, state, f);
    }
  }

  for (auto i : indicesOf(state.markers())) {
    if (handle == scene_handles.markers[i].handle) {
      Optional<BodyIndex> maybe_body_index = state.marker(i).maybe_body_index;

      if (maybe_body_index) {
        forEachSolveFlagAffectingBody(*maybe_body_index, state, f);
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


static bool isScenePath(const TreePath &path, const TreePaths &tree_paths)
{
  return path == tree_paths.path;
}


static bool isBodyPath(const TreePath &path, const TreePaths &tree_paths)
{
  for (const TreePaths::Body &body_paths : tree_paths.bodies) {
    if (body_paths.path == path) {
      return true;
    }
  }

  return false;
}


static Optional<int>
  maybeDistanceErrorIndexOfPath(
    const TreePath &path,
    const TreePaths &tree_paths
  )
{
  for (auto index : indicesOf(tree_paths.distance_errors)) {
    const TreePaths::DistanceError &distance_error_paths =
      tree_paths.distance_errors[index];

    if (path == distance_error_paths.path) {
      return index;
    }
  }

  return {};
}


static Optional<MarkerIndex>
  maybeMarkerIndexOfPath(
    const TreePath &path,
    const TreePaths &tree_paths
  )
{
  for (auto index : indicesOf(tree_paths.markers)) {
    const TreePaths::Marker &marker_paths = tree_paths.markers[index];

    if (path == marker_paths.path) {
      return index;
    }
  }

  return {};
}


struct MainWindowController::Impl {
  struct Data {
    ObservedScene observed_scene;
    Clipboard clipboard;

    Data(Scene &, TreeWidget &);
  };

  Data data_member;

  Impl(Scene &scene, TreeWidget &tree_widget)
  : data_member(scene, tree_widget)
  {
  }

  static bool clipboardIsEmpty(const Data &data)
  {
#if !NEW_CUT_BEHAVIOR
    return data.clipboard.clipboard_tagged_value.children.empty();
#else
    return !data.maybe_cut_body_index.hasValue();
#endif
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

  static void handleSceneChanging(MainWindowController &);
  static void handleSceneChanged(MainWindowController &);

  static void selectMarkerInTree(MarkerIndex marker_index, Data &data)
  {
    data.observed_scene.tree_widget.selectItem(data.observed_scene.tree_paths.markers[marker_index].path);
  }

  static void selectMarker(MarkerIndex marker_index, Data &data)
  {
    selectMarkerInTree(marker_index, data);
    ObservedScene::handleTreeSelectionChanged(data.observed_scene);
  }

  static void
    handleTreeValueChanged(
      MainWindowController &,
      const TreePath &,
      NumericValue
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
      const TreePath &
    );

  static void
    pasteGlobalPressed(MainWindowController &controller, const TreePath &path);

  static MarkerIndex addMarker(MainWindowController &, Optional<BodyIndex>);
  static void addMarkerPressed(MainWindowController &, const TreePath &);
  static void addBodyPressed(MainWindowController &, const TreePath &);
  static void removeBodyPressed(MainWindowController &, const TreePath &);
  static void duplicateBodyPressed(MainWindowController &, const TreePath &);

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
    duplicateMarkerPressed(
      MainWindowController &,
      MarkerIndex
    );

  static void
    cutBodyPressed(
      MainWindowController &,
      const TreePath &
    );

  static void removeMarker(Data &, MarkerIndex);
  static MarkerIndex duplicateMarker(Data &, MarkerIndex);

  static void createMarkerInTree(MarkerIndex marker_index, Data &data)
  {
    TreeWidget &tree_widget = data.observed_scene.tree_widget;
    TreePaths &tree_paths = data.observed_scene.tree_paths;
    SceneState &scene_state = data.observed_scene.scene_state;
    ::createMarkerInTree(tree_widget, tree_paths, scene_state, marker_index);
  }

  static void createMarkerInScene(MarkerIndex marker_index, Data &data)
  {
    SceneHandles &scene_handles = data.observed_scene.scene_handles;
    SceneState &scene_state = data.observed_scene.scene_state;
    Scene &scene = data.observed_scene.scene;
    ::createMarkerInScene(scene, scene_handles, scene_state, marker_index);
  }
};


void
  MainWindowController::Impl::handleSceneChanging(
    MainWindowController &controller
  )
{
  Data &data = Impl::data(controller);
  // The mouse button is down.  The scene is being changed, but we don't
  // consider this change complete.

  Scene &scene = data.observed_scene.scene;
  SceneHandles &scene_handles = data.observed_scene.scene_handles;
  SceneState &state = data.observed_scene.scene_state;
  TreeWidget &tree_widget = data.observed_scene.tree_widget;
  TreePaths &tree_paths = data.observed_scene.tree_paths;
  Optional<TransformHandle> th = scene.selectedObject();
  updateSceneStateFromSceneObjects(state, scene, scene_handles);

  vector<bool> old_flags;

  // Disable any transforms that would move the handle and remember the
  // old state.

  forEachSolveFlagAffectingHandle(
    *th, scene_handles, state,
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
      *th, scene_handles, state,
      [&](bool &arg){
        arg = *iter++;
      }
    );

    assert(iter == old_flags.end());
  }

  updateErrorsInState(state);
  updateTreeValues(tree_widget, tree_paths, state);
  updateSceneObjects(scene, scene_handles, state);
}


void
  MainWindowController::Impl::handleSceneChanged(
    MainWindowController &controller
  )
{
  Data &data = Impl::data(controller);
  SceneHandles &scene_handles = data.observed_scene.scene_handles;
  Scene &scene = data.observed_scene.scene;
  SceneState &state = data.observed_scene.scene_state;
  TreeWidget &tree_widget = data.observed_scene.tree_widget;
  TreePaths &tree_paths = data.observed_scene.tree_paths;

  updateSceneStateFromSceneObjects(state, scene, scene_handles);
  solveScene(state);
  updateErrorsInState(state);
  updateTreeValues(tree_widget, tree_paths, state);
  updateSceneObjects(scene, scene_handles, state);
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
  const TreePaths::Body &body_paths = tree_paths.bodies[body_index];
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
  MainWindowController::Impl::handleTreeValueChanged(
    MainWindowController &controller,
    const TreePath &path,
    NumericValue value
  )
{
  Data &data = Impl::data(controller);
  TreeWidget &tree_widget = data.observed_scene.tree_widget;
  const TreePaths &tree_paths = data.observed_scene.tree_paths;
  SceneState &state = data.observed_scene.scene_state;
  bool value_was_changed = setSceneStateValue(state, path, value, tree_paths);

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

    Scene &scene = data.observed_scene.scene;
    const SceneHandles &scene_handles = data.observed_scene.scene_handles;
    updateErrorsInState(state);
    updateSceneObjects(scene, scene_handles, state);
    updateTreeValues(tree_widget, tree_paths, state);
  }
  else {
    cerr << "Handling spin_box_item_value_changed_function\n";
    cerr << "  path: " << path << "\n";
    cerr << "  value: " << value << "\n";
  }
}


void
  MainWindowController::Impl::handleTreeEnumerationIndexChanged(
    MainWindowController &controller,
    const TreePath &path,
    int value
  )
{
  Data &data = Impl::data(controller);
  const TreePaths &tree_paths = data.observed_scene.tree_paths;
  SceneState &scene_state = data.observed_scene.scene_state;
  setSceneStateEnumerationIndex(scene_state, path, value, tree_paths);
  solveScene(scene_state);
  updateErrorsInState(scene_state);
  updateSceneObjects(data.observed_scene.scene, data.observed_scene.scene_handles, scene_state);
  updateTreeValues(data.observed_scene.tree_widget, data.observed_scene.tree_paths, scene_state);
}


void
  MainWindowController::Impl::addDistanceErrorPressed(
    MainWindowController &controller,
    const TreePath &
  )
{
  Data &data = Impl::data(controller);
  Scene &scene = data.observed_scene.scene;
  SceneHandles &scene_handles = data.observed_scene.scene_handles;
  SceneState &scene_state = data.observed_scene.scene_state;
  TreeWidget &tree_widget = data.observed_scene.tree_widget;
  TreePaths &tree_paths = data.observed_scene.tree_paths;
  DistanceErrorIndex index = scene_state.createDistanceError();

  SceneState::DistanceError &distance_error =
    scene_state.distance_errors[index];

  createDistanceErrorInScene(scene, scene_handles, scene_state, index);

  createDistanceErrorInTree(
    distance_error,
    tree_widget,
    tree_paths,
    scene_state
  );

  tree_widget.selectItem(tree_paths.distance_errors[index].path);

  updateErrorsInState(scene_state);
  updateSceneObjects(scene, scene_handles, scene_state);
  updateTreeValues(tree_widget, tree_paths, scene_state);
}


MarkerIndex
  MainWindowController::Impl::addMarker(
    MainWindowController &controller,
    Optional<BodyIndex> maybe_body_index
  )
{
  Data &data = Impl::data(controller);
  TreePaths &tree_paths = data.observed_scene.tree_paths;
  SceneState &scene_state = data.observed_scene.scene_state;
  TreeWidget &tree_widget = data.observed_scene.tree_widget;
  MarkerIndex marker_index = scene_state.createMarker(maybe_body_index);
  createMarkerInScene(marker_index, data);
  createMarkerInTree(marker_index, data);
  updateTreeDistanceErrorMarkerOptions(tree_widget, tree_paths, scene_state);
  return marker_index;
}


static BodyIndex
  bodyIndexFromTreePath(const TreePath &path, const TreePaths &tree_paths)
{
  for (auto i : indicesOf(tree_paths.bodies)) {
    if (tree_paths.bodies[i].path == path) {
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
  Data &data = Impl::data(controller);
  TreePaths &tree_paths = data.observed_scene.tree_paths;

  Optional<BodyIndex> maybe_body_index =
    maybeBodyIndexFromTreePath(path, tree_paths);;

  MarkerIndex marker_index = addMarker(controller, maybe_body_index);
  selectMarker(marker_index, data);
}


void
MainWindowController::Impl::pasteGlobalPressed(
  MainWindowController &controller,
  const TreePath &path
)
{
  Data &data = Impl::data(controller);
  ObservedScene &observed_scene = data.observed_scene;
  Clipboard &clipboard = data.clipboard;

  Optional<BodyIndex> maybe_new_parent_body_index =
    maybeBodyIndexFromTreePath(path, observed_scene.tree_paths);

  ObservedScene::pasteGlobal(
    maybe_new_parent_body_index, clipboard, observed_scene
  );
}


void
MainWindowController::Impl::addBodyPressed(
  MainWindowController &controller,
  const TreePath &parent_path
)
{
  Data &data = Impl::data(controller);
  SceneState &scene_state = data.observed_scene.scene_state;
  TreePaths &tree_paths = data.observed_scene.tree_paths;

  Optional<BodyIndex> maybe_parent_body_index;

  if (parent_path == tree_paths.path) {
    // Adding a transform to the scene.
  }
  else {
     maybe_parent_body_index = bodyIndexFromTreePath(parent_path, tree_paths);
  }

  BodyIndex body_index =
    createBodyInState(scene_state, maybe_parent_body_index);

  ObservedScene::createBodyInScene(body_index, data.observed_scene);
  ObservedScene::createBodyInTree(body_index, data.observed_scene);
  ObservedScene::selectBody(body_index, data.observed_scene);

}


static BodyIndex duplicateBody(BodyIndex body_index, SceneState &scene_state)
{
  TaggedValue root_tag_value("");
  createBodyTaggedValue(root_tag_value, body_index, scene_state);

  return
    createBodyFromTaggedValue(
      scene_state,
      root_tag_value.children[0],
      scene_state.body(body_index).maybe_parent_index
    );
}


void
MainWindowController::Impl::removeBodyPressed(
  MainWindowController &controller, const TreePath &path
)
{
  Data &data = Impl::data(controller);
  TreeWidget &tree_widget = data.observed_scene.tree_widget;
  TreePaths &tree_paths = data.observed_scene.tree_paths;
  SceneState &scene_state = data.observed_scene.scene_state;
  BodyIndex body_index = bodyIndexFromTreePath(path, tree_paths);
  ObservedScene::removeBody(data.observed_scene, body_index);
  updateTreeDistanceErrorMarkerOptions(tree_widget, tree_paths, scene_state);
}


void
MainWindowController::Impl::duplicateBodyPressed(
  MainWindowController &controller, const TreePath &body_path
)
{
  Data &data = Impl::data(controller);

  Optional<BodyIndex> maybe_body_index =
    bodyIndexFromTreePath(body_path, data.observed_scene.tree_paths);

  assert(maybe_body_index);
  // It should only have been possible to call this for a body.

  BodyIndex body_index = *maybe_body_index;
  BodyIndex new_body_index = duplicateBody(body_index, data.observed_scene.scene_state);
  ObservedScene::createBodyInTree(new_body_index, data.observed_scene);
  ObservedScene::createBodyInScene(new_body_index, data.observed_scene);
  ObservedScene::selectBody(new_body_index, data.observed_scene);
}


void
  MainWindowController::Impl::removeDistanceErrorPressed(
    MainWindowController &controller,
    int distance_error_index
  )
{
  Data &data = Impl::data(controller);
  SceneState &scene_state = data.observed_scene.scene_state;
  Scene &scene = data.observed_scene.scene;
  SceneHandles &scene_handles = data.observed_scene.scene_handles;
  TreePaths &tree_paths = data.observed_scene.tree_paths;
  TreeWidget &tree_widget = data.observed_scene.tree_widget;

  removeDistanceErrorFromTree(
    distance_error_index,
    tree_paths,
    tree_widget
  );

  removeDistanceErrorFromScene(
    scene,
    scene_handles.distance_errors,
    distance_error_index
  );

  scene_state.removeDistanceError(distance_error_index);
  solveScene(scene_state);
  updateTreeValues(tree_widget, tree_paths, scene_state);
  updateSceneObjects(scene, scene_handles, scene_state);
}


void
MainWindowController::Impl::removeMarker(Data &data, MarkerIndex marker_index)
{
  SceneState &scene_state = data.observed_scene.scene_state;
  ObservedScene::removingMarker(data.observed_scene, marker_index);
  scene_state.removeMarker(marker_index);
}


MarkerIndex
MainWindowController::Impl::duplicateMarker(
  Data &data,
  MarkerIndex marker_index
)
{
  SceneState &scene_state = data.observed_scene.scene_state;
  MarkerIndex new_marker_index = scene_state.duplicateMarker(marker_index);
  createMarkerInTree(new_marker_index, data);
  createMarkerInScene(new_marker_index, data);
  return new_marker_index;
}


void
  MainWindowController::Impl::removeMarkerPressed(
    MainWindowController &controller,
    MarkerIndex marker_index
  )
{
  Data &data = Impl::data(controller);
  TreePaths &tree_paths = data.observed_scene.tree_paths;
  TreeWidget &tree_widget = data.observed_scene.tree_widget;
  SceneState &scene_state = data.observed_scene.scene_state;
  removeMarker(data, marker_index);
  updateTreeDistanceErrorMarkerOptions(tree_widget, tree_paths, scene_state);
}


void
  MainWindowController::Impl::duplicateMarkerPressed(
    MainWindowController &controller,
    MarkerIndex source_marker_index
  )
{
  Data &data = Impl::data(controller);
  TreePaths &tree_paths = data.observed_scene.tree_paths;
  TreeWidget &tree_widget = data.observed_scene.tree_widget;
  SceneState &scene_state = data.observed_scene.scene_state;
  MarkerIndex new_marker_index = duplicateMarker(data, source_marker_index);
  updateTreeDistanceErrorMarkerOptions(tree_widget, tree_paths, scene_state);
  selectMarker(new_marker_index, data);
}


template <typename Function>
static void
forEachChildItemPath(
  const TreeWidget &tree_widget,
  const TreePath &parent_path,
  const Function &f
)
{
  int n_children = tree_widget.itemChildCount(parent_path);

  for (int i=0; i!=n_children; ++i) {
    f(childPath(parent_path, i));
  }
}


template <typename Function>
static void
forEachPathInBranch(
  const TreeWidget &tree_widget,
  const TreePath &branch_path,
  const Function &f
)
{
  f(branch_path);

  forEachChildItemPath(tree_widget, branch_path,[&](const TreePath &child_path){
    forEachPathInBranch(tree_widget, child_path, f);
  });
}


#if NEW_CUT_BEHAVIOR
static void
setBranchPending(
  TreeWidget &tree_widget,
  const TreePath &branch_path,
  bool new_state
)
{
  forEachPathInBranch(
    tree_widget, branch_path,
    [&](const TreePath &item_path){
      tree_widget.setItemPending(item_path, new_state);
    }
  );
}
#endif


void
  MainWindowController::Impl::cutBodyPressed(
    MainWindowController &controller,
    const TreePath &path
  )
{
  Data &data = Impl::data(controller);
  TreePaths &tree_paths = data.observed_scene.tree_paths;
  BodyIndex body_index = bodyIndexFromTreePath(path, tree_paths);
  ObservedScene::cutBody(data.observed_scene, body_index, data.clipboard);
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
  Data &data = Impl::data(controller);
  SceneState &state = data.observed_scene.scene_state;
  TreeWidget &tree_widget = data.observed_scene.tree_widget;
  TreePaths &tree_paths = data.observed_scene.tree_paths;
  Scene &scene = data.observed_scene.scene;
  SceneHandles &scene_handles = data.observed_scene.scene_handles;
  flipSolveState(state, path, tree_paths);
  solveScene(state);
  updateErrorsInState(state);
  updateTreeValues(tree_widget, tree_paths, state);
  updateSceneObjects(scene, scene_handles, state);
}


TreeWidget::MenuItems
  MainWindowController::Impl::contextMenuItemsForPath(
    MainWindowController &controller,
    const TreePath &path
  )
{
  Data &data = Impl::data(controller);
  TreeWidget::MenuItems menu_items;
  const TreePaths &tree_paths = data.observed_scene.tree_paths;

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

  if (isScenePath(path, tree_paths)) {
    auto add_distance_error_function =
      [&controller,path]{
        Impl::addDistanceErrorPressed(controller, path);
      };

    appendTo(menu_items,{
      {"Add Distance Error", add_distance_error_function },
      {"Add Marker", add_marker_function },
      {"Add Body", add_body_function },
    });

    if (!clipboardIsEmpty(data)) {
      appendTo(menu_items,{
        {"Paste Global", paste_global_function}
      });
    }
  }

  if (isBodyPath(path, tree_paths)) {
    auto cut_body_function =
      [&controller,path]{
        Impl::cutBodyPressed(controller, path);
      };

    auto remove_body_function =
      [&controller,path]{ Impl::removeBodyPressed(controller, path); };

    auto duplicate_body_function =
      [&controller,path]{ Impl::duplicateBodyPressed(controller, path); };

    appendTo(menu_items,{
      {"Add Marker", add_marker_function},
      {"Add Body", add_body_function},
      {"Cut", cut_body_function },
      {"Remove", remove_body_function },
      {"Duplicate", duplicate_body_function },
    });

    if (!clipboardIsEmpty(data)) {
      appendTo(menu_items,{
        {"Paste Global", paste_global_function}
      });
    }
  }

  {
    Optional<MarkerIndex> maybe_marker_index =
      maybeMarkerIndexOfPath(path, tree_paths);

    if (maybe_marker_index) {
      auto index = *maybe_marker_index;

      auto remove_marker_function = [&controller,index]{
        Impl::removeMarkerPressed(controller, index);
      };

      auto duplicate_marker_function = [&controller,index]{
        Impl::duplicateMarkerPressed(controller, index);
      };

      appendTo(menu_items,{
        {"Remove", remove_marker_function},
        {"Duplicate", duplicate_marker_function}
      });
    }
  }

  {
    Optional<int> maybe_distance_error_index =
      maybeDistanceErrorIndexOfPath(path, tree_paths);

    if (maybe_distance_error_index) {
      auto index = *maybe_distance_error_index;

      auto remove_distance_error_function =
        [&controller,index]{
          Impl::removeDistanceErrorPressed(controller, index);
        };

      appendTo(menu_items,{
        {"Remove Distance Error", remove_distance_error_function}
      });
    }
  }

  if (solveStatePtr(data.observed_scene.scene_state, path, tree_paths)) {
    auto solve_function =
      [&controller,path](){
        Impl::handleSolveToggleChange(controller,path);
      };

    bool checked_state =
      solveState(data.observed_scene.scene_state, path, tree_paths);

    appendTo(menu_items,{
      {"Solve", solve_function, checked_state}
    });
  }

  return menu_items;
}


MainWindowController::Impl::Data::Data(
  Scene &scene_arg,
  TreeWidget &tree_widget_arg
)
: observed_scene(scene_arg, tree_widget_arg)
{
}


MainWindowController::MainWindowController(
  Scene &scene,
  TreeWidget &tree_widget
)
: impl_ptr(new Impl(scene, tree_widget))
{
  Impl::Data &data = Impl::data(*this);
  SceneHandles &scene_handles = data.observed_scene.scene_handles;
  SceneState &state = data.observed_scene.scene_state;
  updateSceneStateFromSceneObjects(state, scene, scene_handles);
  scene.changed_callback = [&]{ Impl::handleSceneChanged(*this); };
  scene.changing_callback = [&]{ Impl::handleSceneChanging(*this); };

  scene.selection_changed_callback =
    [this]{ ObservedScene::handleSceneSelectionChanged(Impl::data(*this).observed_scene); };

  tree_widget.spin_box_item_value_changed_callback =
    [this](const TreePath &path, NumericValue value){
      Impl::handleTreeValueChanged(*this, path, value);
    };

  tree_widget.enumeration_item_index_changed_callback =
    [this](const TreePath &path, int index){
      Impl::handleTreeEnumerationIndexChanged(*this, path, index);
    };

  tree_widget.selection_changed_callback =
    [&data](){
      ObservedScene::handleTreeSelectionChanged(data.observed_scene);
    };

  tree_widget.context_menu_items_callback =
    [this](const TreePath &path){
      return Impl::contextMenuItemsForPath(*this, path);
    };
}


MainWindowController::~MainWindowController() = default;


void MainWindowController::replaceSceneStateWith(const SceneState &new_state)
{
  Impl::Data &data = Impl::data(*this);
  Scene &scene = data.observed_scene.scene;
  SceneHandles &scene_handles = data.observed_scene.scene_handles;
  TreeWidget &tree_widget = data.observed_scene.tree_widget;
  SceneState &scene_state = data.observed_scene.scene_state;
  destroySceneObjects(scene, scene_state, scene_handles);
  clearTree(tree_widget, data.observed_scene.tree_paths);
  scene_state = new_state;

  solveScene(scene_state);
  scene_handles = createSceneObjects(scene_state, scene);
  data.observed_scene.tree_paths = fillTree(tree_widget, scene_state);
}


const SceneState &MainWindowController::sceneState()
{
  Impl::Data &data = Impl::data(*this);
  return data.observed_scene.scene_state;
}
