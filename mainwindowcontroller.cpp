#include "mainwindowcontroller.hpp"

#include "streamvector.hpp"
#include "sceneerror.hpp"
#include "scenesolver.hpp"
#include "treevalues.hpp"
#include "startswith.hpp"
#include "rotationvector.hpp"
#include "indicesof.hpp"
#include "sceneobjects.hpp"
#include "maketransform.hpp"
#include "matchconst.hpp"
#include "scenestatetaggedvalue.hpp"
#include "globaltransform.hpp"

#define NEW_CUT_BEHAVIOR 0

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


template <typename F>
static void
  forEachTransformHandlePath(
    const F &f,
    const SceneHandles &scene_handles,
    const TreePaths &tree_paths
  )
{
  Optional<TransformHandle> maybe_object;

  for (auto i : indicesOf(tree_paths.markers)) {
    f(scene_handles.markers[i].handle, tree_paths.markers[i].path);
  }

  for (auto i : indicesOf(tree_paths.bodies)) {
    f(scene_handles.bodies[i], tree_paths.bodies[i].path);
  }

  for (auto i : indicesOf(tree_paths.distance_errors)) {
    f(
      scene_handles.distance_errors[i].line,
      tree_paths.distance_errors[i].path
    );
  }
}


static Optional<TransformHandle>
  sceneObjectForTreeItem(
    const TreePath &item_path,
    const TreePaths &tree_paths,
    const SceneHandles &scene_handles
  )
{
  TreePath matching_path;
  Optional<TransformHandle> maybe_matching_handle;

  forEachTransformHandlePath(
    [&](TransformHandle object_handle, const TreePath &object_path){
      if (startsWith(item_path, object_path)) {
        if (object_path.size() > matching_path.size()) {
          matching_path = object_path;
          maybe_matching_handle = object_handle;
        }
      }
    },
    scene_handles,
    tree_paths
  );

  return maybe_matching_handle;
}


static Optional<TreePath>
  treeItemForSceneObject(
    TransformHandle handle,
    const TreePaths &tree_paths,
    const SceneHandles &scene_handles
  )
{
  Optional<TreePath> maybe_found_path;

  forEachTransformHandlePath(
    [&](TransformHandle object_handle, const TreePath &object_path){
      if (object_handle == handle) {
        maybe_found_path = object_path;
      }
    },
    scene_handles,
    tree_paths
  );

  return maybe_found_path;
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
    SceneState scene_state;
    Scene &scene;
    TreeWidget &tree_widget;

    SceneHandles scene_handles;
    TreePaths tree_paths;
#if !NEW_CUT_BEHAVIOR
    TaggedValue clipboard_tagged_value;
    Transform clipboard_transform;
#else
    Optional<BodyIndex> maybe_cut_body_index;
#endif

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
    return data.clipboard_tagged_value.children.empty();
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
  static void handleTreeSelectionChanged(Data &);

  static void createBodyInTree(BodyIndex body_index, Data &data)
  {
    TreeWidget &tree_widget = data.tree_widget;
    TreePaths &tree_paths = data.tree_paths;
    SceneState &scene_state = data.scene_state;
    ::createBodyInTree(tree_widget, tree_paths, scene_state, body_index);
  }

  static void createBodyInScene(BodyIndex body_index, Data &data)
  {
    SceneHandles &scene_handles = data.scene_handles;
    SceneState &scene_state = data.scene_state;
    Scene &scene = data.scene;
    ::createBodyInScene(scene, scene_handles, scene_state, body_index);
  }

  static void selectMarkerInTree(MarkerIndex marker_index, Data &data)
  {
    data.tree_widget.selectItem(data.tree_paths.markers[marker_index].path);
  }

  static void selectBodyInTree(BodyIndex body_index, Data &data)
  {
    TreeWidget &tree_widget = data.tree_widget;
    TreePaths &tree_paths = data.tree_paths;
    tree_widget.selectItem(tree_paths.bodies[body_index].path);
  }

  static void selectMarker(MarkerIndex marker_index, Data &data)
  {
    selectMarkerInTree(marker_index, data);
    handleTreeSelectionChanged(data);
  }

  static void selectBody(BodyIndex body_index, Data &data)
  {
    selectBodyInTree(body_index, data);
    handleTreeSelectionChanged(data);
  }

  static bool isRotateItem(const TreePath &, const TreePaths &);
  static bool isScaleItem(const TreePath &, const TreePaths &);

  static void attachProperDraggerToSelectedObject(Data &);
  static void handleSceneSelectionChanged(MainWindowController &controller);

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

  static void removingMarker(Data &, MarkerIndex);
  static void removingBody(Data &, BodyIndex);

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

  static void removeBody(Data &, BodyIndex);

  static void removeMarker(Data &, MarkerIndex);
  static MarkerIndex duplicateMarker(Data &, MarkerIndex);

  static void createMarkerInTree(MarkerIndex marker_index, Data &data)
  {
    TreeWidget &tree_widget = data.tree_widget;
    TreePaths &tree_paths = data.tree_paths;
    SceneState &scene_state = data.scene_state;
    ::createMarkerInTree(tree_widget, tree_paths, scene_state, marker_index);
  }

  static void createMarkerInScene(MarkerIndex marker_index, Data &data)
  {
    SceneHandles &scene_handles = data.scene_handles;
    SceneState &scene_state = data.scene_state;
    Scene &scene = data.scene;
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

  Scene &scene = data.scene;
  SceneHandles &scene_handles = data.scene_handles;
  SceneState &state = data.scene_state;
  TreeWidget &tree_widget = data.tree_widget;
  TreePaths &tree_paths = data.tree_paths;
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
  SceneHandles &scene_handles = data.scene_handles;
  Scene &scene = data.scene;
  SceneState &state = data.scene_state;
  TreeWidget &tree_widget = data.tree_widget;
  TreePaths &tree_paths = data.tree_paths;

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
  TreeWidget &tree_widget = data.tree_widget;
  const TreePaths &tree_paths = data.tree_paths;
  SceneState &state = data.scene_state;
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

    Scene &scene = data.scene;
    const SceneHandles &scene_handles = data.scene_handles;
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
  const TreePaths &tree_paths = data.tree_paths;
  SceneState &scene_state = data.scene_state;
  setSceneStateEnumerationIndex(scene_state, path, value, tree_paths);
  solveScene(scene_state);
  updateErrorsInState(scene_state);
  updateSceneObjects(data.scene, data.scene_handles, scene_state);
  updateTreeValues(data.tree_widget, data.tree_paths, scene_state);
}


void
  MainWindowController::Impl::addDistanceErrorPressed(
    MainWindowController &controller,
    const TreePath &
  )
{
  Data &data = Impl::data(controller);
  Scene &scene = data.scene;
  SceneHandles &scene_handles = data.scene_handles;
  SceneState &scene_state = data.scene_state;
  TreeWidget &tree_widget = data.tree_widget;
  TreePaths &tree_paths = data.tree_paths;
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
  TreePaths &tree_paths = data.tree_paths;
  SceneState &scene_state = data.scene_state;
  TreeWidget &tree_widget = data.tree_widget;
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
  TreePaths &tree_paths = data.tree_paths;

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
  SceneState &scene_state = data.scene_state;

  Optional<BodyIndex> maybe_new_parent_body_index =
    maybeBodyIndexFromTreePath(path, data.tree_paths);

#if !NEW_CUT_BEHAVIOR
  const TaggedValue &body_tagged_value =
    data.clipboard_tagged_value.children[0];

  const Transform &old_parent_global_transform =
    data.clipboard_transform;
#else
  BodyIndex old_body_index = *data.maybe_cut_body_index;
  data.maybe_cut_body_index.reset();
  TaggedValue clipboard("clipboard");

  Optional<BodyIndex> maybe_old_parent_body_index =
    scene_state.body(old_body_index).maybe_parent_index;

  createBodyTaggedValue(clipboard, old_body_index, scene_state);

  Transform old_parent_global_transform =
    globalTransform(maybe_old_parent_body_index, scene_state);

  const TaggedValue &body_tagged_value = clipboard.children[0];

  removeBody(data, old_body_index);

  if (maybe_new_parent_body_index) {
    if (*maybe_new_parent_body_index > old_body_index) {
      --*maybe_new_parent_body_index;
    }
  }
#endif

  BodyIndex new_body_index =
    createBodyFromTaggedValue(
      scene_state,
      body_tagged_value,
      maybe_new_parent_body_index
    );

  SceneState::Body &body_state = scene_state.body(new_body_index);
  Transform old_body_transform = makeTransformFromState(body_state.transform);

  Transform new_parent_global_transform =
    globalTransform(maybe_new_parent_body_index, scene_state);

  Transform body_global_transform = old_parent_global_transform*old_body_transform;
  Transform new_body_transform = new_parent_global_transform.inverse()*body_global_transform;
  body_state.transform = transformState(new_body_transform);

  createBodyInTree(new_body_index, data);
  createBodyInScene(new_body_index, data);
  selectBody(new_body_index, data);
}


void
MainWindowController::Impl::addBodyPressed(
  MainWindowController &controller,
  const TreePath &parent_path
)
{
  Data &data = Impl::data(controller);
  SceneState &scene_state = data.scene_state;
  TreePaths &tree_paths = data.tree_paths;

  Optional<BodyIndex> maybe_parent_body_index;

  if (parent_path == tree_paths.path) {
    // Adding a transform to the scene.
  }
  else {
     maybe_parent_body_index = bodyIndexFromTreePath(parent_path, tree_paths);
  }

  BodyIndex body_index =
    createBodyInState(scene_state, maybe_parent_body_index);

  createBodyInScene(body_index, data);
  createBodyInTree(body_index, data);
  selectBody(body_index, data);

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
  TreeWidget &tree_widget = data.tree_widget;
  TreePaths &tree_paths = data.tree_paths;
  SceneState &scene_state = data.scene_state;
  BodyIndex body_index = bodyIndexFromTreePath(path, tree_paths);
  removeBody(data, body_index);
  updateTreeDistanceErrorMarkerOptions(tree_widget, tree_paths, scene_state);
}


void
MainWindowController::Impl::duplicateBodyPressed(
  MainWindowController &controller, const TreePath &body_path
)
{
  Data &data = Impl::data(controller);

  Optional<BodyIndex> maybe_body_index =
    bodyIndexFromTreePath(body_path, data.tree_paths);

  assert(maybe_body_index);
  // It should only have been possible to call this for a body.

  BodyIndex body_index = *maybe_body_index;
  BodyIndex new_body_index = duplicateBody(body_index, data.scene_state);
  createBodyInTree(new_body_index, data);
  createBodyInScene(new_body_index, data);
  selectBody(new_body_index, data);
}


void
  MainWindowController::Impl::removeDistanceErrorPressed(
    MainWindowController &controller,
    int distance_error_index
  )
{
  Data &data = Impl::data(controller);
  SceneState &scene_state = data.scene_state;
  Scene &scene = data.scene;
  SceneHandles &scene_handles = data.scene_handles;
  TreePaths &tree_paths = data.tree_paths;
  TreeWidget &tree_widget = data.tree_widget;

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
MainWindowController::Impl::removingMarker(Data &data, MarkerIndex marker_index)
{
  TreePaths &tree_paths = data.tree_paths;
  TreeWidget &tree_widget = data.tree_widget;
  SceneHandles &scene_handles = data.scene_handles;
  Scene &scene = data.scene;
  removeMarkerFromTree(marker_index, tree_paths, tree_widget);
  removeMarkerFromScene(scene, scene_handles.markers, marker_index);
}


void MainWindowController::Impl::removingBody(Data &data, BodyIndex body_index)
{
  Scene &scene = data.scene;
  SceneHandles &scene_handles = data.scene_handles;
  SceneState &scene_state = data.scene_state;
  TreePaths &tree_paths = data.tree_paths;
  TreeWidget &tree_widget = data.tree_widget;
  removeBodyFromScene(scene, scene_handles, scene_state, body_index);
  removeBodyFromTree(tree_widget, tree_paths, scene_state, body_index);
}


void
MainWindowController::Impl::removeMarker(Data &data, MarkerIndex marker_index)
{
  SceneState &scene_state = data.scene_state;
  removingMarker(data, marker_index);
  scene_state.removeMarker(marker_index);
}


MarkerIndex
MainWindowController::Impl::duplicateMarker(
  Data &data,
  MarkerIndex marker_index
)
{
  SceneState &scene_state = data.scene_state;
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
  TreePaths &tree_paths = data.tree_paths;
  TreeWidget &tree_widget = data.tree_widget;
  SceneState &scene_state = data.scene_state;
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
  TreePaths &tree_paths = data.tree_paths;
  TreeWidget &tree_widget = data.tree_widget;
  SceneState &scene_state = data.scene_state;
  MarkerIndex new_marker_index = duplicateMarker(data, source_marker_index);
  updateTreeDistanceErrorMarkerOptions(tree_widget, tree_paths, scene_state);
  selectMarker(new_marker_index, data);
}


void MainWindowController::Impl::removeBody(Data &data, BodyIndex body_index)
{
  auto removing_marker_function =
    [&](MarkerIndex arg){ removingMarker(data, arg); };

  auto removing_body_function =
    [&](BodyIndex arg){ removingBody(data, arg); };

  removeBodyFromSceneState(
    body_index,
    data.scene_state,
    removing_marker_function,
    removing_body_function
  );
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
  TreeWidget &tree_widget = data.tree_widget;
  TreePaths &tree_paths = data.tree_paths;
  BodyIndex body_index = bodyIndexFromTreePath(path, tree_paths);
#if !NEW_CUT_BEHAVIOR
  SceneState &scene_state = data.scene_state;

  Optional<BodyIndex> maybe_parent_body_index =
    scene_state.body(body_index).maybe_parent_index;

  data.clipboard_tagged_value.children.clear();
  createBodyTaggedValue(data.clipboard_tagged_value, body_index, scene_state);

  data.clipboard_transform =
    globalTransform(maybe_parent_body_index, scene_state);

  removeBody(data, body_index);
  updateTreeDistanceErrorMarkerOptions(tree_widget, tree_paths, scene_state);
#else
  if (data.maybe_cut_body_index) {
    setBranchPending(
      tree_widget,
      tree_paths.bodies[*data.maybe_cut_body_index].path,
      false
    );

    data.maybe_cut_body_index.reset();
  }

  setBranchPending(tree_widget, path, true);
  data.maybe_cut_body_index = body_index;
#endif
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
  SceneState &state = data.scene_state;
  TreeWidget &tree_widget = data.tree_widget;
  TreePaths &tree_paths = data.tree_paths;
  Scene &scene = data.scene;
  SceneHandles &scene_handles = data.scene_handles;
  flipSolveState(state, path, data.tree_paths);
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
  const TreePaths &tree_paths = data.tree_paths;

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

  if (solveStatePtr(data.scene_state, path, tree_paths)) {
    auto solve_function =
      [&controller,path](){
        Impl::handleSolveToggleChange(controller,path);
      };

    bool checked_state =
      solveState(data.scene_state, path, tree_paths);

    appendTo(menu_items,{
      {"Solve", solve_function, checked_state}
    });
  }

  return menu_items;
}


bool
  MainWindowController::Impl::isRotateItem(
    const TreePath &path,
    const TreePaths &tree_paths
  )
{
  for (auto i : indicesOf(tree_paths.bodies)) {
    if (startsWith(path, tree_paths.bodies[i].rotation.path)) {
      return true;
    }
  }

  return false;
}


bool
  MainWindowController::Impl::isScaleItem(
    const TreePath &path,
    const TreePaths &tree_paths
  )
{
  for (auto i : indicesOf(tree_paths.bodies)) {
    if (startsWith(path, tree_paths.bodies[i].geometry.path)) {
      return true;
    }
  }

  return false;
}


void
  MainWindowController::Impl::attachProperDraggerToSelectedObject(
    Data &data
  )
{
  Scene &scene = data.scene;
  TreeWidget &tree_widget = data.tree_widget;
  TreePaths &tree_paths = data.tree_paths;

  Optional<Scene::LineHandle> maybe_line_handle =
    scene.maybeLine(*scene.selectedObject());

  if (!maybe_line_handle) {
    if (isRotateItem(*tree_widget.selectedItem(), tree_paths)) {
      scene.attachDraggerToSelectedNode(Scene::DraggerType::rotate);
    }
    else if (isScaleItem(*tree_widget.selectedItem(), tree_paths)) {
      scene.attachDraggerToSelectedNode(Scene::DraggerType::scale);
    }
    else {
      scene.attachDraggerToSelectedNode(Scene::DraggerType::translate);
    }
  }
}


void MainWindowController::Impl::handleTreeSelectionChanged(Data &data)
{
  TreeWidget &tree_widget = data.tree_widget;
  Scene &scene = data.scene;
  Optional<TreePath> maybe_selected_item_path = tree_widget.selectedItem();

  if (!maybe_selected_item_path) {
    cerr << "No tree item selected\n";
    return;
  }

  Optional<TransformHandle> maybe_object =
    sceneObjectForTreeItem(
      *maybe_selected_item_path, data.tree_paths, data.scene_handles
    );

  if (maybe_object) {
    scene.selectObject(*maybe_object);
    attachProperDraggerToSelectedObject(data);
  }
  else {
    cerr << "No scene object found\n";
  }
}


void
  MainWindowController::Impl::handleSceneSelectionChanged(
    MainWindowController &controller
  )
{
  Data &data = Impl::data(controller);
  Scene &scene = data.scene;
  TreeWidget &tree_widget = data.tree_widget;

  Optional<TransformHandle> maybe_selected_transform_handle =
    scene.selectedObject();

  if (!maybe_selected_transform_handle) {
    cerr << "No object selected in the scene.\n";
    return;
  }

  TransformHandle selected_transform_handle =
    *maybe_selected_transform_handle;

  Optional<TreePath> maybe_tree_path =
    treeItemForSceneObject(
      selected_transform_handle,
      data.tree_paths,
      data.scene_handles
    );

  if (maybe_tree_path) {
    tree_widget.selectItem(*maybe_tree_path);
  }
  else {
    cerr << "No tree item for scene object.\n";
  }

  attachProperDraggerToSelectedObject(data);
}


MainWindowController::Impl::Data::Data(
  Scene &scene_arg,
  TreeWidget &tree_widget_arg
)
:
  scene(scene_arg),
  tree_widget(tree_widget_arg),
  scene_handles(createSceneObjects(scene_state, scene)),
  tree_paths(fillTree(tree_widget,scene_state))
#if !NEW_CUT_BEHAVIOR
  ,
  clipboard_tagged_value("clipboard")
#endif
{
}


MainWindowController::MainWindowController(
  Scene &scene,
  TreeWidget &tree_widget
)
: impl_ptr(new Impl(scene, tree_widget))
{
  Impl::Data &data = Impl::data(*this);
  SceneHandles &scene_handles = data.scene_handles;
  SceneState &state = data.scene_state;
  updateSceneStateFromSceneObjects(state, scene, scene_handles);
  scene.changed_callback = [&]{ Impl::handleSceneChanged(*this); };
  scene.changing_callback = [&]{ Impl::handleSceneChanging(*this); };

  scene.selection_changed_callback =
    [this]{ Impl::handleSceneSelectionChanged(*this); };

  tree_widget.spin_box_item_value_changed_callback =
    [this](const TreePath &path, NumericValue value){
      Impl::handleTreeValueChanged(*this, path, value);
    };

  tree_widget.enumeration_item_index_changed_callback =
    [this](const TreePath &path, int index){
      Impl::handleTreeEnumerationIndexChanged(*this, path, index);
    };

  tree_widget.selection_changed_callback =
    [&data](){ Impl::handleTreeSelectionChanged(data); };

  tree_widget.context_menu_items_callback =
    [this](const TreePath &path){
      return Impl::contextMenuItemsForPath(*this, path);
    };
}


MainWindowController::~MainWindowController() = default;


void MainWindowController::replaceSceneStateWith(const SceneState &new_state)
{
  Impl::Data &data = Impl::data(*this);
  Scene &scene = data.scene;
  SceneHandles &scene_handles = data.scene_handles;
  TreeWidget &tree_widget = data.tree_widget;
  SceneState &scene_state = data.scene_state;
  destroySceneObjects(scene, scene_state, scene_handles);
  clearTree(tree_widget, data.tree_paths);
  scene_state = new_state;

  solveScene(scene_state);
  scene_handles = createSceneObjects(scene_state, scene);
  data.tree_paths = fillTree(tree_widget, scene_state);
}


const SceneState &MainWindowController::sceneState()
{
  Impl::Data &data = Impl::data(*this);
  return data.scene_state;
}
