#include "mainwindowcontroller.hpp"

#include "streamvector.hpp"
#include "sceneerror.hpp"
#include "scenesolver.hpp"
#include "defaultscenestate.hpp"
#include "treevalues.hpp"
#include "startswith.hpp"
#include "rotationvector.hpp"
#include "indicesof.hpp"
#include "sceneobjects.hpp"

using std::cerr;
using TransformHandle = Scene::TransformHandle;


static bool
  movesWithBox(
    TransformHandle handle,
    const SceneHandles &setup,
    const SceneState &state
  )
{
  if (handle == setup.box) {
    return true;
  }

  MarkerIndex n_markers = state.markers().size();

  for (MarkerIndex i=0; i!=n_markers; ++i) {
    if (handle == setup.markers[i].handle) {
      if (state.marker(i).is_local) {
        return true;
      }
    }
  }

  return false;
}


static void
  setVectorValue(
    Eigen::Vector3f &v,
    const TreePath &path,
    NumericValue value,
    const TreePaths::XYZ &xyz_path
  )
{
  if (path == xyz_path.x) {
    v.x() = value;
  }
  else if (path == xyz_path.y) {
    v.y() = value;
  }
  else if (path == xyz_path.z) {
    v.z() = value;
  }
  else {
    assert(false);
  }
}


static void
  setVectorValue(
    Vec3 &v,
    const TreePath &path,
    NumericValue value,
    const TreePaths::XYZ &xyz_path
  )
{
  if (path == xyz_path.x) {
    v.x = value;
  }
  else if (path == xyz_path.y) {
    v.y = value;
  }
  else if (path == xyz_path.z) {
    v.z = value;
  }
  else {
    assert(false);
  }
}


static bool
  setTransformValue(
    Transform &box_global,
    const TreePath &path,
    NumericValue value,
    const TreePaths::Box &box_paths
  )
{
  if (startsWith(path,box_paths.translation.path)) {
    Eigen::Vector3f v = box_global.translation();
    const TreePaths::XYZ& xyz_path = box_paths.translation;
    setVectorValue(v, path, value, xyz_path);
    box_global.translation() = v;
    return true;
  }

  if (startsWith(path,box_paths.rotation.path)) {
    Vec3 v = rotationVector(box_global.rotation());
    const TreePaths::XYZ& xyz_path = box_paths.rotation;
    setVectorValue(v, path, value*M_PI/180, xyz_path);
    box_global.matrix().topLeftCorner<3,3>() = makeRotation(v);
    return true;
  }

  return false;
}


static bool
  setMarkerValue(
    const TreePath &path,
    NumericValue value,
    const TreePaths::Marker &marker_path,
    SceneState::Marker &marker_state
  )
{
  if (startsWith(path,marker_path.position.path)) {
    setVectorValue(marker_state.position, path, value, marker_path.position);
    return true;
  }

  return false;
}


static bool
  setDistanceErrorValue(
    SceneState::DistanceError &distance_error_state,
    const TreePath &path,
    NumericValue value,
    const TreePaths::DistanceError &distance_error_paths
  )
{
  if (startsWith(path, distance_error_paths.desired_distance)) {
    distance_error_state.desired_distance = value;
    return true;
  }

  if (startsWith(path, distance_error_paths.weight)) {
    distance_error_state.weight = value;
    return true;
  }

  return false;
}


static bool
  setDistanceErrorsValue(
    SceneState::DistanceErrors &distance_error_states,
    const TreePath &path,
    NumericValue value,
    const TreePaths::DistanceErrors &distance_errors_paths
  )
{
  assert(distance_errors_paths.size() == distance_error_states.size());

  for (auto i : indicesOf(distance_errors_paths)) {
    bool value_was_set =
      setDistanceErrorValue(
        distance_error_states[i],
        path,
        value,
        distance_errors_paths[i]
      );

    if (value_was_set) {
      return true;
    }
  }

  return false;
}


static bool
  setMarkersValue(
    const TreePath &path,
    NumericValue value,
    const TreePaths::Markers &markers_paths,
    SceneState &scene_state
  )
{
  for (auto i : indicesOf(markers_paths)) {
    bool value_was_set =
      setMarkerValue(
        path,
        value,
        markers_paths[i],
        scene_state.marker(i)
      );

    if (value_was_set) {
      return true;
    }
  }

  return false;
}



static bool
  setSceneStateValue(
    SceneState &scene_state,
    const TreePath &path,
    NumericValue value,
    const TreePaths &tree_paths
  )
{
  const TreePaths::Box &box_paths = tree_paths.box;

  if (startsWith(path,box_paths.path)) {
    Transform box_global = scene_state.box.global;

    if (setTransformValue(box_global, path, value, box_paths)) {
      scene_state.box.global = box_global;
      return true;
    }

    if (startsWith(path, box_paths.geometry.path)) {
      SceneState::Box &box = scene_state.box;
      Eigen::Vector3f v = {box.scale_x, box.scale_y, box.scale_z};
      const TreePaths::XYZ& xyz_path = box_paths.geometry.scale;
      setVectorValue(v, path, value, xyz_path);
      box.scale_x = v.x();
      box.scale_y = v.y();
      box.scale_z = v.z();
      return true;
    }
  }

  {
    bool was_markers_value =
      setMarkersValue(
        path,
        value,
        tree_paths.markers,
        scene_state
      );

    if (was_markers_value) {
      return true;
    }
  }

  {
    bool was_distance_error_value =
      setDistanceErrorsValue(
        scene_state.distance_errors,
        path,
        value,
        tree_paths.distance_errors
      );

    if (was_distance_error_value) {
      return true;
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


static Optional<TransformHandle>
  sceneObjectForTreeItem(
    const TreePath &item_path,
    const TreePaths &tree_paths,
    const SceneHandles &scene_handles
  )
{
  Optional<TransformHandle> maybe_object;

  for (auto i : indicesOf(tree_paths.markers)) {
    if (startsWith(item_path, tree_paths.markers[i].path)) {
      return scene_handles.markers[i].handle;
    }
  }

  if (startsWith(item_path, tree_paths.box.path)) {
    return scene_handles.box;
  }

  for (auto i : indicesOf(tree_paths.distance_errors)) {
    if (startsWith(item_path, tree_paths.distance_errors[i].path)) {
      return scene_handles.distance_errors[i].line;
    }
  }

  return {};
}


static Optional<TreePath>
  treeItemForSceneObject(
    TransformHandle handle,
    const TreePaths &tree_paths,
    const SceneHandles &scene_handles
  )
{
  Optional<TreePath> maybe_tree_path;

  if (scene_handles.box == handle) {
    return tree_paths.box.path;
  }

  for (auto i : indicesOf(scene_handles.markers)) {
    if (scene_handles.markers[i].handle == handle) {
      return tree_paths.markers[i].path;
    }
  }

  for (auto i : indicesOf(scene_handles.distance_errors)) {
    if (scene_handles.distance_errors[i].line == handle) {
      return tree_paths.distance_errors[i].path;
    }
  }

  return {};
}


static bool isScenePath(const TreePath &path, const TreePaths &tree_paths)
{
  return path == tree_paths.path;
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
    addDistanceErrorPressed(
      MainWindowController &controller,
      const TreePath &
    );

  static void
    addMarkerPressed(
      MainWindowController &controller,
      const TreePath &
    );

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

  if (!th || !movesWithBox(*th,scene_handles,state)) {
    // If we're moving something that doesn't move with the box, then
    // we'll go ahead and update the box position.
    solveBoxPosition(state);
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
  solveBoxPosition(state);
  updateErrorsInState(state);
  updateTreeValues(tree_widget, tree_paths, state);
  updateSceneObjects(scene, scene_handles, state);
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
    bool is_box_transform_path =
      startsWith(path, tree_paths.box.translation.path) ||
      startsWith(path, tree_paths.box.rotation.path);

    if (!is_box_transform_path) {
      solveBoxPosition(state);
    }

    Scene &scene = data.scene;
    const SceneHandles &scene_handles = data.scene_handles;
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
  solveBoxPosition(scene_state);
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
  SceneState::DistanceError &distance_error = scene_state.addDistanceError();
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
  MainWindowController::Impl::addMarkerPressed(
    MainWindowController &controller,
    const TreePath &path
  )
{
  TreePaths &tree_paths = controller.data.tree_paths;
  SceneState &scene_state = controller.data.scene_state;
  Scene &scene = controller.data.scene;
  SceneHandles &scene_handles = controller.data.scene_handles;
  TreeWidget &tree_widget = controller.data.tree_widget;

  if (isScenePath(path, tree_paths)) {
    MarkerIndex marker_index =
      createMarkerInState(scene_state, /*is_local*/false);

    createMarkerInScene(scene, scene_handles, scene_state, marker_index);
    createMarkerInTree(tree_widget, tree_paths, scene_state, marker_index);
    updateTreeDistanceErrorMarkerOptions(tree_widget, tree_paths, scene_state);
  }
  else {
    cerr << "Add marker to something else\n";
  }
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
  solveBoxPosition(scene_state);
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


TreeWidget::MenuItems
  MainWindowController::Impl::contextMenuItemsForPath(
    MainWindowController &controller,
    const TreePath &path
  )
{
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

    return {
      {"Add Distance Error", add_distance_error_function },
      {"Add Marker", add_marker_error_function },
    };
  }

  {
    Optional<MarkerIndex> maybe_marker_index =
      maybeMarkerIndexOfPath(path, tree_paths);

    if (maybe_marker_index) {
      auto index = *maybe_marker_index;

      auto remove_marker_function = [&controller,index]{
        Impl::removeMarkerPressed(controller, index);
      };

      return {
        {"Remove Marker", remove_marker_function}
      };
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

      return {
        {"Remove Distance Error", remove_distance_error_function}
      };
    }
  }

  return {};
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
  Scene &scene_arg,
  TreeWidget &tree_widget_arg
)
: scene(scene_arg),
  tree_widget(tree_widget_arg),
  scene_state(defaultSceneState()),
  scene_handles(createSceneObjects(scene_state, scene)),
  tree_paths(fillTree(tree_widget,scene_state))
{
}


MainWindowController::MainWindowController(Scene &scene,TreeWidget &tree_widget)
: data{scene,tree_widget}
{
  TreePaths &tree_paths = data.tree_paths;
  SceneHandles &scene_handles = data.scene_handles;
  SceneState &state = data.scene_state;
  updateSceneStateFromSceneObjects(state, scene, scene_handles);
  solveBoxPosition(state);
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
