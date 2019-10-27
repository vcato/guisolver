#include "mainwindowcontroller.hpp"

#include <iostream>
#include "streamvector.hpp"
#include "transform.hpp"
#include "eigenconv.hpp"
#include "maketransform.hpp"
#include "scenestate.hpp"
#include "sceneerror.hpp"
#include "scenesolver.hpp"
#include "setupscene.hpp"
#include "treevalues.hpp"
#include "startswith.hpp"
#include "rotationvector.hpp"
#include "indicesof.hpp"
#include "updatescenestatefromscene.hpp"
#include "settransform.hpp"
#include "createsceneobjects.hpp"

using std::cerr;


static void
  updateDistanceErrorInScene(
    Scene &scene,
    const SceneHandles::DistanceError &distance_error_handles,
    const SceneState::DistanceError &distance_error_state,
    const SceneHandles &scene_handles
  )
{
  Scene::TransformHandle line_start =
    scene_handles.markers[
      distance_error_state.start_marker_index
    ].handle;

  Scene::TransformHandle line_end =
    scene_handles.markers[
      distance_error_state.end_marker_index
    ].handle;

  Scene::Point start = scene.worldPoint({0,0,0}, line_start);
  Scene::Point end = scene.worldPoint({0,0,0}, line_end);
  scene.setStartPoint(distance_error_handles.line, start);
  scene.setEndPoint(distance_error_handles.line, end);
}


static void
  updateDistanceErrorsInScene(
    Scene &scene,
    const SceneHandles &scene_handles,
    const SceneState &scene_state
  )
{
  for (auto i : indicesOf(scene_state.distance_errors))
  {
    updateDistanceErrorInScene(
      scene,
      scene_handles.distance_errors[i],
      scene_state.distance_errors[i],
      scene_handles
    );
  }
}


static bool
  movesWithBox(
    Scene::TransformHandle th,
    const SceneHandles &setup,
    const SceneState &state
  )
{
  if (th == setup.box) {
    return true;
  }

  MarkerIndex n_markers = state.markers.size();

  for (MarkerIndex i=0; i!=n_markers; ++i) {
    if (th == setup.markers[i].handle) {
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
  Optional<Scene::TransformHandle> th = scene.selectedObject();
  SceneHandles &scene_handles = main_window_data.scene_handles;
  SceneState &state = main_window_data.scene_state;
  updateSceneStateFromScene(state, scene, scene_handles);

  if (!th || !movesWithBox(*th,scene_handles,state)) {
    // If we're moving something that doesn't move with the box, then
    // we'll go ahead and update the box position.
    solveBoxPosition(state);
    setTransform(scene_handles.box, state.box_global, scene);
  }
  else {
    // If we're moving something that moves with the box, then it will
    // be confusing if we try to update the box position.
  }

  updateDistanceErrorsInState(state);
  updateTreeValues(main_window_data);
  updateDistanceErrorsInScene(scene, scene_handles, state);
}


static void sceneChangedCallback(MainWindowData &main_window_data)
{
  SceneHandles &scene_handles = main_window_data.scene_handles;
  Scene &scene = main_window_data.scene;
  SceneState &state = main_window_data.scene_state;
  updateSceneStateFromScene(state, scene, scene_handles);
  solveBoxPosition(state);
  setTransform(scene_handles.box, state.box_global, scene);
  updateDistanceErrorsInState(state);
  updateTreeValues(main_window_data);
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
    const TreePaths::DistanceError &distance_error_path
  )
{
  if (startsWith(path, distance_error_path.desired_distance)) {
    distance_error_state.desired_distance = value;
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
    const SceneHandles &scene_setup
  )
{
  const TreePaths::Box &box_paths = tree_paths.box;

  if (startsWith(path,box_paths.path)) {
    Transform box_global = scene_state.box_global;

    if (setTransformValue(box_global, path, value, box_paths)) {
      setTransform(scene_setup.box, box_global, scene);
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
        scene_setup.markers
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
  updateSceneStateFromScene(state, scene, scene_handles);

  bool scene_value_was_changed =
    setSceneValue(state, scene, path, value, tree_paths, scene_handles);

  if (scene_value_was_changed) {
    bool is_box_transform_path =
      startsWith(path, tree_paths.box.translation.path) ||
      startsWith(path, tree_paths.box.rotation.path);

    if (!is_box_transform_path) {
      updateSceneStateFromScene(state, scene, scene_handles);
      solveBoxPosition(state);
      setTransform(scene_handles.box, state.box_global, scene);
    }

    updateDistanceErrorsInScene(scene, scene_handles, state);
    updateTreeValues(main_window_data);
  }
  else {
    cerr << "Handling spin_box_item_value_changed_function\n";
    cerr << "  path: " << path << "\n";
    cerr << "  value: " << value << "\n";
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


static void updateBoxPositionInScene(MainWindowData &main_window_data)
{
  SceneHandles &scene_handles = main_window_data.scene_handles;
  SceneState &state = main_window_data.scene_state;
  Scene &scene = main_window_data.scene;
  setTransform(scene_handles.box, state.box_global, scene);
}


static void sceneSelectionChanged(MainWindowData &main_window_data)
{
  Scene &scene = main_window_data.scene;
  SceneHandles &scene_handles = main_window_data.scene_handles;
  TreeWidget &tree_widget = main_window_data.tree_widget;
  TreePaths &tree_paths = main_window_data.tree_paths;

  Optional<Scene::TransformHandle> maybe_selected_transform_handle =
    scene.selectedObject();

  if (!maybe_selected_transform_handle) {
    cerr << "No object selected in the scene.\n";
    return;
  }

  Scene::TransformHandle selected_transform_handle =
    *maybe_selected_transform_handle;

  if (scene_handles.box == selected_transform_handle) {
    tree_widget.selectItem(tree_paths.box.path);
  }

  for (auto i : indicesOf(scene_handles.markers)) {
    if (scene_handles.markers[i].handle == selected_transform_handle) {
      tree_widget.selectItem(tree_paths.markers[i].path);
    }
  }
}


MainWindowController::MainWindowController(Scene &scene,TreeWidget &tree_widget)
: main_window_data{scene,tree_widget}
{
  SceneHandles &scene_handles = main_window_data.scene_handles;
  SceneState &state = main_window_data.scene_state;
  updateSceneStateFromScene(state, scene, scene_handles);
  solveBoxPosition(state);
  updateDistanceErrorsInState(state);
  updateBoxPositionInScene(main_window_data);
  updateDistanceErrorsInScene(scene, scene_handles, state);
  updateTreeValues(main_window_data);
  scene.changed_callback = [&]{ sceneChangedCallback(main_window_data); };
  scene.changing_callback = [&]{ sceneChangingCallback(main_window_data); };

  scene.selection_changed_callback =
    [&]{ sceneSelectionChanged(main_window_data); };

  tree_widget.spin_box_item_value_changed_function =
    [&](const TreePath &path, NumericValue value){
      treeValueChangedCallback(main_window_data, path, value);
    };
}
