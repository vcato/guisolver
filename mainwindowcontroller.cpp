#include "mainwindowcontroller.hpp"

#include <iostream>
#include "streamvector.hpp"
#include "transform.hpp"
#include "eigenconv.hpp"
#include "maketransform.hpp"
#include "scenestate.hpp"
#include "updatetreevalues.hpp"
#include "sceneerror.hpp"
#include "scenesolver.hpp"
#include "setupscene.hpp"
#include "filltree.hpp"
#include "startswith.hpp"
#include "rotationvector.hpp"

using std::cerr;


static void
  setTransform(
    Scene::TransformHandle transform_id,
    const Transform &transform_value,
    Scene &scene
  )
{
  CoordinateAxes coordinate_axes =
    coordinateAxes(transform_value.rotation());

  scene.setCoordinateAxes(transform_id, coordinate_axes);
  scene.setTranslation(transform_id, transform_value.translation());
}


static Transform
  globalTransform(const Scene &scene,Scene::TransformHandle transform_id)
{
  Point translation = scene.translation(transform_id);
  CoordinateAxes coordinate_axes = scene.coordinateAxes(transform_id);
  return makeTransform(coordinate_axes,translation);
}


static Point
  localTranslation(Scene::TransformHandle transform_id,const Scene &scene)
{
  return scene.translation(transform_id);
}


static void
  addStateMarker(
    SceneState &result,
    const SceneSetup::Marker &setup_marker,
    const Scene &scene
  )
{
  Point position = localTranslation(setup_marker.handle, scene);

  if (setup_marker.is_local) {
    result.addLocalMarker(position);
  }
  else {
    result.addGlobalMarker(position);
  }
}


static void
  addStateLine(
    SceneState &result,
    const SceneSetup::Line &setup_line,
    const Scene &
  )
{
  MarkerIndex start_marker_index = setup_line.start_marker_index;
  MarkerIndex end_marker_index = setup_line.end_marker_index;
  result.addLine(start_marker_index,end_marker_index);
}


static SceneState makeSceneState(const Scene &scene,const SceneSetup &setup)
{
  SceneState result;

  for (const auto &setup_marker : setup.markers) {
    addStateMarker(result, setup_marker, scene);
  }

  for (const auto &setup_line : setup.lines) {
    addStateLine(result, setup_line, scene);
  }

  result.box_global = globalTransform(scene,setup.box);

  return result;
}


static void updateLines(Scene &scene,const SceneSetup &setup)
{
  for (auto &line : setup.lines) {
    Scene::TransformHandle line_start =
      setup.markers[line.start_marker_index].handle;

    Scene::TransformHandle line_end =
      setup.markers[line.end_marker_index].handle;

    Scene::Point start = scene.worldPoint({0,0,0}, line_start);
    Scene::Point end = scene.worldPoint({0,0,0}, line_end);
    scene.setStartPoint(line.handle,start);
    scene.setEndPoint(line.handle,end);
  }
}


static void showError(const Scene &scene, const SceneSetup &setup)
{
  cerr << sceneError(makeSceneState(scene,setup)) << "\n";
}


static bool
  movesWithBox(
    Scene::TransformHandle th,
    const SceneSetup &setup,
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


static void sceneChangingCallback(MainWindowData &main_window_data)
{
  // The mouse button is down.  The scene is being changed, but we don't
  // consider this change complete.

  Scene &scene = main_window_data.scene;
  Optional<Scene::TransformHandle> th = scene.selectedObject();
  SceneSetup &setup = main_window_data.scene_setup;
  SceneState state = makeSceneState(scene,setup);

  if (!th || !movesWithBox(*th,setup,state)) {
    // If we're moving something that doesn't move with the box, then
    // we'll go ahead and update the box position.
    solveBoxPosition(state);
    setTransform(setup.box, state.box_global, scene);
  }
  else {
    // If we're moving something that moves with the box, then it will
    // be confusing if we try to update the box position.
  }

  updateLines(scene,setup);
  TreeWidget &tree_widget = main_window_data.tree_widget;
  TreePaths &tree_paths = main_window_data.tree_paths;
  updateTreeValues(tree_widget, tree_paths, state);
  showError(scene, setup);
}


static void sceneChangedCallback(MainWindowData &main_window_data)
{
  SceneSetup &setup = main_window_data.scene_setup;
  Scene &scene = main_window_data.scene;
  SceneState state = makeSceneState(scene,setup);
  solveBoxPosition(state);
  setTransform(setup.box, state.box_global, scene);

  TreePaths &tree_paths = main_window_data.tree_paths;
  TreeWidget &tree_widget = main_window_data.tree_widget;
  updateTreeValues(tree_widget, tree_paths, state);

  updateLines(scene,setup);
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
  setMarkersValue(
    Scene &scene,
    const TreePath &path,
    NumericValue value,
    const TreePaths::Markers &markers_paths,
    const SceneSetup::Markers &setup_markers
  )
{
  assert(markers_paths.size() == setup_markers.size());
  int n_markers = markers_paths.size();

  for (int i=0; i!=n_markers; ++i) {
    const TreePaths::Marker &marker_paths = markers_paths[i];
    Scene::TransformHandle marker_handle = setup_markers[i].handle;

    if (startsWith(path,marker_paths.position.path)) {
      Scene::Point v = scene.translation(marker_handle);
      setVectorValue(v, path, value, marker_paths.position);
      scene.setTranslation(marker_handle, v);
      return true;
    }
  }

  return false;
}


static bool
  setSceneValue(
    Scene &scene,
    const TreePath &path,
    NumericValue value,
    const TreePaths &tree_paths,
    const SceneSetup &scene_setup
  )
{
  SceneState scene_state = makeSceneState(scene, scene_setup);

  const TreePaths::Box &box_paths = tree_paths.box;

  if (startsWith(path,box_paths.path)) {
    Transform box_global = scene_state.box_global;

    if (setTransformValue(box_global, path, value, box_paths)) {
      setTransform(scene_setup.box, box_global, scene);
      return true;
    }
  }

  {
    const TreePaths::Markers &markers_paths = tree_paths.markers;

    if (
      setMarkersValue(scene, path, value, markers_paths, scene_setup.markers)
    ) {
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
  const SceneSetup &scene_setup = main_window_data.scene_setup;

  if (setSceneValue(scene, path, value, tree_paths, scene_setup)) {
    bool is_box_transform_path =
      startsWith(path, tree_paths.box.translation.path) ||
      startsWith(path, tree_paths.box.rotation.path);

    if (!is_box_transform_path) {
      SceneState state = makeSceneState(scene,scene_setup);
      solveBoxPosition(state);
      setTransform(scene_setup.box, state.box_global, scene);
    }

    updateLines(scene, scene_setup);
    showError(scene, scene_setup);
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
  scene_setup(setupScene(scene)),
  tree_paths(fillTree(tree_widget))
{
}


MainWindowController::MainWindowController(Scene &scene,TreeWidget &tree_widget)
: main_window_data{scene,tree_widget}
{
  TreePaths &tree_paths = main_window_data.tree_paths;
  SceneSetup &scene_setup = main_window_data.scene_setup;

  SceneState state = makeSceneState(scene,scene_setup);
  solveBoxPosition(state);
  setTransform(scene_setup.box, state.box_global, scene);
  updateLines(scene, scene_setup);

  updateTreeValues(tree_widget,tree_paths,state);

  scene.changed_callback = [&]{ sceneChangedCallback(main_window_data); };
  scene.changing_callback = [&]{ sceneChangingCallback(main_window_data); };

  tree_widget.spin_box_item_value_changed_function =
    [&](const TreePath &path, NumericValue value){
      treeValueChangedCallback(main_window_data, path, value);
    };
}
