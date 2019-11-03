#include "mainwindowcontroller.hpp"

#include "streamvector.hpp"
#include "eigenconv.hpp"
#include "sceneerror.hpp"
#include "scenesolver.hpp"
#include "defaultscenestate.hpp"
#include "treevalues.hpp"
#include "startswith.hpp"
#include "rotationvector.hpp"
#include "indicesof.hpp"
#include "updatescenestatefromscene.hpp"
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

  MarkerIndex n_markers = state.markers.size();

  for (MarkerIndex i=0; i!=n_markers; ++i) {
    if (handle == setup.markers[i].handle) {
      if (state.markers[i].is_local) {
        return true;
      }
    }
  }

  return false;
}


static void updateTreeValues(MainWindowData &main_window_data)
{
  TreeWidget &tree_widget = main_window_data.tree_widget;
  TreePaths &tree_paths = main_window_data.tree_paths;
  SceneState &state = main_window_data.scene_state;
  updateTreeValues(tree_widget, tree_paths, state);
}


static void sceneChangingCallback(MainWindowData &main_window_data)
{
  // The mouse button is down.  The scene is being changed, but we don't
  // consider this change complete.

  Scene &scene = main_window_data.scene;
  Optional<TransformHandle> th = scene.selectedObject();
  SceneHandles &scene_handles = main_window_data.scene_handles;
  SceneState &state = main_window_data.scene_state;
  updateSceneStateFromScene(state, scene, scene_handles);

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
  updateTreeValues(main_window_data);
  updateBoxInScene(scene, scene_handles.box, state.box);
  updateDistanceErrorsInScene(scene, scene_handles, state);
}


static void sceneChangedCallback(MainWindowData &main_window_data)
{
  SceneHandles &scene_handles = main_window_data.scene_handles;
  Scene &scene = main_window_data.scene;
  SceneState &state = main_window_data.scene_state;
  updateSceneStateFromScene(state, scene, scene_handles);
  solveBoxPosition(state);
  updateErrorsInState(state);
  updateTreeValues(main_window_data);
  updateBoxInScene(scene, scene_handles.box, state.box);
  updateDistanceErrorsInScene(scene,scene_handles,state);
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
    Scene &scene,
    const TreePath &path,
    NumericValue value,
    const TreePaths::Marker &marker_path,
    const SceneHandles::Marker &setup_marker
  )
{
  if (startsWith(path,marker_path.position.path)) {
    Scene::Point v = scene.translation(setup_marker.handle);
    setVectorValue(v, path, value, marker_path.position);
    scene.setTranslation(setup_marker.handle, v);
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
    Scene &scene,
    const TreePath &path,
    NumericValue value,
    const TreePaths::Markers &markers_paths,
    const SceneHandles::Markers &setup_markers
  )
{
  assert(markers_paths.size() == setup_markers.size());

  for (auto i : indicesOf(markers_paths)) {
    if (setMarkerValue(scene,path,value,markers_paths[i],setup_markers[i])) {
      return true;
    }
  }

  return false;
}



static bool
  setSceneValue(
    SceneState &scene_state,
    Scene &scene,
    const TreePath &path,
    NumericValue value,
    const TreePaths &tree_paths,
    const SceneHandles &scene_handles
  )
{
  const TreePaths::Box &box_paths = tree_paths.box;

  if (startsWith(path,box_paths.path)) {
    Transform box_global = scene_state.box.global;

    if (setTransformValue(box_global, path, value, box_paths)) {
      scene_state.box.global = box_global;
      updateBoxInScene(scene, scene_handles.box, scene_state.box);
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
        scene,
        path,
        value,
        tree_paths.markers,
        scene_handles.markers
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
  treeValueChangedCallback(
    MainWindowData &main_window_data,
    const TreePath &path,
    NumericValue value
  )
{
  const TreePaths &tree_paths = main_window_data.tree_paths;
  Scene &scene = main_window_data.scene;
  const SceneHandles &scene_handles = main_window_data.scene_handles;
  SceneState &state = main_window_data.scene_state;

  bool scene_value_was_changed =
    setSceneValue(state, scene, path, value, tree_paths, scene_handles);

  if (scene_value_was_changed) {
    bool is_box_transform_path =
      startsWith(path, tree_paths.box.translation.path) ||
      startsWith(path, tree_paths.box.rotation.path);

    if (!is_box_transform_path) {
      updateSceneStateFromScene(state, scene, scene_handles);
      solveBoxPosition(state);
    }

    updateBoxInScene(scene, scene_handles.box, state.box);
    updateDistanceErrorsInScene(scene, scene_handles, state);
    updateTreeValues(main_window_data);
  }
  else {
    cerr << "Handling spin_box_item_value_changed_function\n";
    cerr << "  path: " << path << "\n";
    cerr << "  value: " << value << "\n";
  }
}


static Optional<TransformHandle>
  sceneObjectForTreeItem(
    MainWindowData &main_window_data,
    const TreePath &item_path
  )
{
  TreePaths &tree_paths = main_window_data.tree_paths;
  SceneHandles &scene_handles = main_window_data.scene_handles;
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


static void treeSelectionChangedCallback(MainWindowData &main_window_data)
{
  TreeWidget &tree_widget = main_window_data.tree_widget;
  Scene &scene = main_window_data.scene;
  Optional<TreePath> maybe_selected_item_path = tree_widget.selectedItem();

  if (!maybe_selected_item_path) {
    cerr << "No tree item selected\n";
    return;
  }

  Optional<TransformHandle> maybe_object =
    sceneObjectForTreeItem(main_window_data, *maybe_selected_item_path);

  if (maybe_object) {
    scene.selectObject(*maybe_object);
  }
}


static Optional<TreePath>
  treeItemForSceneObject(
    MainWindowData &main_window_data,
    TransformHandle handle
  )
{
  SceneHandles &scene_handles = main_window_data.scene_handles;
  TreePaths &tree_paths = main_window_data.tree_paths;
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


static void sceneSelectionChanged(MainWindowData &main_window_data)
{
  Scene &scene = main_window_data.scene;
  TreeWidget &tree_widget = main_window_data.tree_widget;

  Optional<TransformHandle> maybe_selected_transform_handle =
    scene.selectedObject();

  if (!maybe_selected_transform_handle) {
    cerr << "No object selected in the scene.\n";
    return;
  }

  TransformHandle selected_transform_handle =
    *maybe_selected_transform_handle;

  Optional<TreePath> maybe_tree_path =
    treeItemForSceneObject(main_window_data, selected_transform_handle);

  if (maybe_tree_path) {
    tree_widget.selectItem(*maybe_tree_path);
  }
  else {
    cerr << "No tree item for scene object.\n";
  }
}


MainWindowData::MainWindowData(
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
: main_window_data{scene,tree_widget}
{
  SceneHandles &scene_handles = main_window_data.scene_handles;
  SceneState &state = main_window_data.scene_state;
  updateSceneStateFromScene(state, scene, scene_handles);
  solveBoxPosition(state);
  updateErrorsInState(state);
  updateBoxInScene(scene, scene_handles.box, state.box);
  updateDistanceErrorsInScene(scene, scene_handles, state);
  updateTreeValues(main_window_data);
  scene.changed_callback = [&]{ sceneChangedCallback(main_window_data); };
  scene.changing_callback = [&]{ sceneChangingCallback(main_window_data); };

  scene.selection_changed_callback =
    [this]{ sceneSelectionChanged(main_window_data); };

  tree_widget.spin_box_item_value_changed_callback =
    [this](const TreePath &path, NumericValue value){
      treeValueChangedCallback(main_window_data, path, value);
    };

  tree_widget.selection_changed_callback =
    [this](){ treeSelectionChangedCallback(main_window_data); };
}
