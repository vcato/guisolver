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


static bool anyFlagsAreSet(const SceneState::XYZSolveFlags &solve_flags)
{
  return solve_flags.x || solve_flags.y || solve_flags.z;
}


static bool anyFlagsAreSet(const SceneState::TransformSolveFlags &solve_flags)
{
  return
    anyFlagsAreSet(solve_flags.translation) ||
    anyFlagsAreSet(solve_flags.rotation);
}


static bool
  bodyIsSolved(
    BodyIndex body_index,
    const SceneState &state
  )
{
  const SceneState::Body &body_state = state.body(body_index);
  return anyFlagsAreSet(body_state.solve_flags);
}


static bool bodyIsAffectedBySolve(BodyIndex body_index, const SceneState &state)
{
  if (bodyIsSolved(body_index, state)) {
    return true;
  }

  Optional<BodyIndex> maybe_parent_index =
    state.body(body_index).maybe_parent_index;

  if (!maybe_parent_index) {
    return false;
  }

  return bodyIsAffectedBySolve(*maybe_parent_index, state);
}


static bool
  affectedBySolve(
    TransformHandle handle,
    const SceneHandles &scene_handles,
    const SceneState &state
  )
{
  // If the handle is for a body that is a child of the box, then it
  // is going to be affected by the solve.
  for (auto i : indicesOf(state.bodies())) {
    if (handle == scene_handles.bodies[i]) {
      return bodyIsAffectedBySolve(i, state);
    }
  }

  for (auto i : indicesOf(state.markers())) {
    if (handle == scene_handles.markers[i].handle) {
      Optional<BodyIndex> maybe_body_index = state.marker(i).maybe_body_index;

      if (maybe_body_index) {
        return bodyIsAffectedBySolve(*maybe_body_index, state);
      }
    }
  }

  return false;
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


static bool isTransformPath(const TreePath &path, const TreePaths &tree_paths)
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
  static void handleSceneChanging(MainWindowController &);
  static void handleSceneChanged(MainWindowController &);
  static void handleTreeSelectionChanged(MainWindowController &controller);
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

  static void addMarker(MainWindowController &, Optional<BodyIndex>);
  static void addMarkerPressed(MainWindowController &, const TreePath &);

  static void
    addTransformPressed(MainWindowController &, const TreePath &);

  static void
    removeDistanceErrorPressed(
      MainWindowController &controller,
      int distance_error_index
    );

  static void
    removeMarkerPressed(
      MainWindowController &controller,
      MarkerIndex
    );
};


void
  MainWindowController::Impl::handleSceneChanging(
    MainWindowController &controller
  )
{
  Data &data = controller.data;
  // The mouse button is down.  The scene is being changed, but we don't
  // consider this change complete.

  Scene &scene = data.scene;
  SceneHandles &scene_handles = data.scene_handles;
  SceneState &state = data.scene_state;
  TreeWidget &tree_widget = data.tree_widget;
  TreePaths &tree_paths = data.tree_paths;
  Optional<TransformHandle> th = scene.selectedObject();
  updateSceneStateFromSceneObjects(state, scene, scene_handles);

  if (!th || !affectedBySolve(*th,scene_handles,state)) {
    // If we're moving something that doesn't move with the box, then
    // we'll go ahead and update the box position.
    solveScene(state);
  }
  else {
    // If we're moving something that moves with the box, then it will
    // be confusing if we try to update the box position.
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
  Data &data = controller.data;
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


static const bool *
  xyzSolveStatePtr(
    const SceneState::XYZSolveFlags &xyz_solve_flags,
    const TreePath &path,
    const TreePaths::XYZ &xyz_paths
  )
{
  if (path == xyz_paths.x) return &xyz_solve_flags.x;
  if (path == xyz_paths.y) return &xyz_solve_flags.y;
  if (path == xyz_paths.z) return &xyz_solve_flags.z;
  return 0;
}


static const bool *
  bodySolveStatePtr(
    const SceneState &scene_state,
    const TreePath &path,
    const TreePaths &tree_paths,
    BodyIndex body_index
  )
{
  const SceneState::Body &body_state = scene_state.body(body_index);
  const TreePaths::Body &body_paths = tree_paths.bodies[body_index];
  {
    const TreePaths::XYZ &xyz_paths = body_paths.translation;

    const SceneState::XYZSolveFlags &xyz_solve_flags =
      body_state.solve_flags.translation;

    const bool *result_ptr = xyzSolveStatePtr(xyz_solve_flags, path, xyz_paths);

    if (result_ptr) {
      return result_ptr;
    }
  }
  {
    const TreePaths::XYZ &xyz_paths = body_paths.rotation;

    const SceneState::XYZSolveFlags &xyz_solve_flags =
      body_state.solve_flags.rotation;

    const bool *result_ptr = xyzSolveStatePtr(xyz_solve_flags, path, xyz_paths);

    if (result_ptr) {
      return result_ptr;
    }
  }

  return nullptr;
}


static const bool *
  solveStatePtr(
    const SceneState &scene_state,
    const TreePath &path,
    const TreePaths &tree_paths
  )
{
  for (auto body_index : indicesOf(scene_state.bodies())) {
    const bool *solve_state_ptr =
      bodySolveStatePtr(scene_state, path, tree_paths, body_index);

    if (solve_state_ptr) {
      return solve_state_ptr;
    }
  }

  return nullptr;
}


static bool
  valueIsSolved(
    const TreePath &path,
    const SceneState &scene_state,
    const TreePaths &tree_paths
  )
{
  const bool *solve_state_ptr = solveStatePtr(scene_state, path, tree_paths);

  if (!solve_state_ptr) {
    // Value is not solvable, so it definitely isn't solved.
    return false;
  }

  // Value is solvable, just have to see whether it is set to be solved
  // or not.
  return *solve_state_ptr;
}


void
  MainWindowController::Impl::handleTreeValueChanged(
    MainWindowController &controller,
    const TreePath &path,
    NumericValue value
  )
{
  Data &data = controller.data;
  TreeWidget &tree_widget = data.tree_widget;
  const TreePaths &tree_paths = data.tree_paths;
  SceneState &state = data.scene_state;
  bool value_was_changed = setSceneStateValue(state, path, value, tree_paths);

  if (value_was_changed) {
    bool value_is_solved = valueIsSolved(path, state, tree_paths);

    // If the value is solved, then we don't want to re-solve the box
    // position immediately because that would give strange feedback to
    // the user.  It would feel like they didn't have control of the value.
    // However, if the value is not solved, then it makes sense to go ahead
    // and solve the box position, since the user's value will not be
    // affected.

    if (!value_is_solved) {
      solveScene(state);
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
  Data &data = controller.data;
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
  Scene &scene = controller.data.scene;
  SceneHandles &scene_handles = controller.data.scene_handles;
  SceneState &scene_state = controller.data.scene_state;
  TreeWidget &tree_widget = controller.data.tree_widget;
  TreePaths &tree_paths = controller.data.tree_paths;
  SceneState::DistanceError &distance_error = scene_state.createDistanceError();
  createDistanceErrorInScene(scene, scene_handles, distance_error);

  createDistanceErrorInTree(
    distance_error,
    tree_widget,
    tree_paths,
    scene_state
  );

  updateErrorsInState(scene_state);
  updateSceneObjects(scene, scene_handles, scene_state);
  updateTreeValues(tree_widget, tree_paths, scene_state);
}


void
  MainWindowController::Impl::addMarker(
    MainWindowController &controller,
    Optional<BodyIndex> maybe_body_index
  )
{
  TreePaths &tree_paths = controller.data.tree_paths;
  SceneState &scene_state = controller.data.scene_state;
  Scene &scene = controller.data.scene;
  SceneHandles &scene_handles = controller.data.scene_handles;
  TreeWidget &tree_widget = controller.data.tree_widget;
  MarkerIndex marker_index = createMarkerInState(scene_state, maybe_body_index);
  createMarkerInScene(scene, scene_handles, scene_state, marker_index);
  createMarkerInTree(tree_widget, tree_paths, scene_state, marker_index);
  updateTreeDistanceErrorMarkerOptions(tree_widget, tree_paths, scene_state);
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


void
  MainWindowController::Impl::addMarkerPressed(
    MainWindowController &controller,
    const TreePath &path
  )
{
  TreePaths &tree_paths = controller.data.tree_paths;

  if (tree_paths.path == path) {
    addMarker(controller, /*maybe_body_index*/{});
  }
  else {
    Optional<BodyIndex> maybe_body_index =
      bodyIndexFromTreePath(path, tree_paths);

    addMarker(controller, maybe_body_index);
  }
}


void
MainWindowController::Impl::addTransformPressed(
  MainWindowController &controller,
  const TreePath &parent_path
)
{
  SceneState &scene_state = controller.data.scene_state;
  Scene &scene = controller.data.scene;
  SceneHandles &scene_handles = controller.data.scene_handles;
  TreePaths &tree_paths = controller.data.tree_paths;
  TreeWidget &tree_widget = controller.data.tree_widget;

  BodyIndex parent_body_index =
    bodyIndexFromTreePath(parent_path, tree_paths);

  BodyIndex index = createBodyInState(scene_state, parent_body_index);
  createBodyInScene(scene, scene_handles, scene_state, index);
  createBodyInTree(tree_widget, tree_paths, scene_state, index);
}


void
  MainWindowController::Impl::removeDistanceErrorPressed(
    MainWindowController &controller,
    int distance_error_index
  )
{
  SceneState &scene_state = controller.data.scene_state;
  Scene &scene = controller.data.scene;
  SceneHandles &scene_handles = controller.data.scene_handles;
  TreePaths &tree_paths = controller.data.tree_paths;
  TreeWidget &tree_widget = controller.data.tree_widget;

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
  MainWindowController::Impl::removeMarkerPressed(
    MainWindowController &controller,
    MarkerIndex marker_index
  )
{
  TreePaths &tree_paths = controller.data.tree_paths;
  TreeWidget &tree_widget = controller.data.tree_widget;
  Scene &scene = controller.data.scene;
  SceneHandles &scene_handles = controller.data.scene_handles;
  SceneState &scene_state = controller.data.scene_state;
  removeMarkerFromTree(marker_index, tree_paths, tree_widget);
  removeMarkerFromScene(scene, scene_handles.markers, marker_index);
  scene_state.removeMarker(marker_index);
  updateTreeDistanceErrorMarkerOptions(tree_widget, tree_paths, scene_state);
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
  SceneState &state = controller.data.scene_state;
  TreeWidget &tree_widget = controller.data.tree_widget;
  TreePaths &tree_paths = controller.data.tree_paths;
  Scene &scene = controller.data.scene;
  SceneHandles &scene_handles = controller.data.scene_handles;

  flipSolveState(
    state,
    path,
    controller.data.tree_paths
  );

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
  TreeWidget::MenuItems menu_items;
  const TreePaths &tree_paths = controller.data.tree_paths;

  if (isScenePath(path, tree_paths)) {
    auto add_distance_error_function =
      [&controller,path]{
        Impl::addDistanceErrorPressed(controller, path);
      };

    auto add_marker_error_function =
      [&controller,path]{
        Impl::addMarkerPressed(controller, path);
      };

    appendTo(menu_items,{
      {"Add Distance Error", add_distance_error_function },
      {"Add Marker", add_marker_error_function },
    });
  }

  if (isTransformPath(path, tree_paths)) {
    auto add_marker_function =
      [&controller,path]{
        Impl::addMarkerPressed(controller, path);
      };

    auto add_transform_function =
      [&controller,path]{ Impl::addTransformPressed(controller, path); };

    appendTo(menu_items,{
      {"Add Marker", add_marker_function},
      {"Add Transform", add_transform_function}
    });
  }

  {
    Optional<MarkerIndex> maybe_marker_index =
      maybeMarkerIndexOfPath(path, tree_paths);

    if (maybe_marker_index) {
      auto index = *maybe_marker_index;

      auto remove_marker_function = [&controller,index]{
        Impl::removeMarkerPressed(controller, index);
      };

      appendTo(menu_items,{
        {"Remove Marker", remove_marker_function}
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

  if (solveStatePtr(controller.data.scene_state, path, tree_paths)) {
    auto solve_function =
      [&controller,path](){
        Impl::handleSolveToggleChange(controller,path);
      };

    bool checked_state =
      solveState(controller.data.scene_state, path, tree_paths);

    appendTo(menu_items,{
      {"Solve", solve_function, checked_state}
    });
  }

  return menu_items;
}


void
  MainWindowController::Impl::handleTreeSelectionChanged(
    MainWindowController &controller
  )
{
  Data &data = controller.data;
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
  Data &data = controller.data;
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
}


MainWindowController::Data::Data(
  SceneState &scene_state_arg,
  Scene &scene_arg,
  TreeWidget &tree_widget_arg
)
: scene_state(scene_state_arg),
  scene(scene_arg),
  tree_widget(tree_widget_arg),
  scene_handles(createSceneObjects(scene_state, scene)),
  tree_paths(fillTree(tree_widget,scene_state))
{
}


MainWindowController::MainWindowController(
  SceneState &scene_state,
  Scene &scene,
  TreeWidget &tree_widget
)
: data{scene_state, scene, tree_widget}
{
  TreePaths &tree_paths = data.tree_paths;
  SceneHandles &scene_handles = data.scene_handles;
  SceneState &state = data.scene_state;
  updateSceneStateFromSceneObjects(state, scene, scene_handles);
  solveScene(state);
  updateErrorsInState(state);
  updateSceneObjects(scene, scene_handles, state);
  updateTreeValues(tree_widget, tree_paths, state);
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
    [this](){ Impl::handleTreeSelectionChanged(*this); };

  tree_widget.context_menu_items_callback =
    [this](const TreePath &path){
      return Impl::contextMenuItemsForPath(*this, path);
    };
}


void MainWindowController::notifySceneStateChanged()
{
  Scene &scene = data.scene;
  SceneHandles &scene_handles = data.scene_handles;
  TreeWidget &tree_widget = data.tree_widget;
  SceneState &scene_state = data.scene_state;
  destroySceneObjects(scene, scene_handles);
  clearTree(tree_widget, data.tree_paths);
  scene_handles = createSceneObjects(scene_state, scene);
  data.tree_paths = fillTree(tree_widget, scene_state);
}
