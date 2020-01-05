#include "observedscene.hpp"

#include "startswith.hpp"
#include "scenestatetaggedvalue.hpp"
#include "treevalues.hpp"
#include "sceneobjects.hpp"
#include "sceneerror.hpp"
#include "scenestatetransform.hpp"

using TransformHandle = Scene::TransformHandle;
using GeometryHandle = Scene::GeometryHandle;
using std::cerr;


namespace {
struct SceneObject {
  Optional<TransformHandle> maybe_transform_handle;
  Optional<GeometryHandle> maybe_geometry_handle;

  SceneObject() = default;

  SceneObject(
    Optional<TransformHandle> maybe_transform_handle,
    Optional<GeometryHandle> maybe_geometry_handle
  )
  : maybe_transform_handle(maybe_transform_handle),
    maybe_geometry_handle(maybe_geometry_handle)
  {
  }
};
}


template <typename F>
static void
forEachSceneObjectPath(
  const F &f,
  const SceneHandles &scene_handles,
  const TreePaths &tree_paths
)
{
  Optional<TransformHandle> maybe_object;

  for (auto i : indicesOf(tree_paths.markers)) {
    const SceneHandles::Marker &marker_handles = scene_handles.marker(i);

    f(
      {
        {}, // marker_handles.transformHandle(),
        marker_handles.sphereHandle(),
      },
      tree_paths.marker(i).path
    );
  }

  for (auto body_index : indicesOf(tree_paths.bodies)) {
    const SceneHandles::Body &body_handles = scene_handles.body(body_index);
    const TreePaths::Body &body_paths = tree_paths.body(body_index);

    assert(body_handles.boxes.size() == body_paths.boxes.size());
    size_t n_boxes = body_handles.boxes.size();

    f(
      {
        body_handles.transformHandle(),
        Optional<GeometryHandle>{},
      },
      body_paths.path
    );

    for (size_t box_index = 0; box_index != n_boxes; ++box_index) {
      f(
        {
          body_handles.transformHandle(),
          body_handles.boxes[box_index].handle,
        },
        body_paths.boxes[box_index].path
      );
    }
  }

  for (auto i : indicesOf(tree_paths.distance_errors)) {
    const SceneHandles::DistanceError &distance_error_handles =
      scene_handles.distance_errors[i];

    f(
      {
        distance_error_handles.transform_handle,
        distance_error_handles.line_handle,
      },
      tree_paths.distance_errors[i].path
    );
  }
}


static SceneObject
sceneObjectForTreeItem(
  const TreePath &item_path,
  const TreePaths &tree_paths,
  const SceneHandles &scene_handles
)
{
  TreePath matching_path;
  SceneObject scene_object;

  forEachSceneObjectPath(
    [&](const SceneObject &object, const TreePath &object_path){
      const Optional<GeometryHandle> &maybe_geometry_handle =
        object.maybe_geometry_handle;

      if (startsWith(item_path, object_path)) {
        if (object_path.size() > matching_path.size()) {
          matching_path = object_path;

          if (maybe_geometry_handle) {
            scene_object.maybe_geometry_handle = *maybe_geometry_handle;
          }
          else {
            scene_object.maybe_transform_handle = object.maybe_transform_handle;
          }
        }
      }
    },
    scene_handles,
    tree_paths
  );

  return scene_object;
}


static Optional<TreePath>
treeItemForSceneObject(
  const SceneObject &scene_object,
  const TreePaths &tree_paths,
  const SceneHandles &scene_handles
)
{
  Optional<TreePath> maybe_found_path;

  forEachSceneObjectPath(
    [&](const SceneObject &object, const TreePath &object_path){
      const Optional<GeometryHandle> &maybe_geometry_handle =
        object.maybe_geometry_handle;

      if (scene_object.maybe_geometry_handle) {
        if (scene_object.maybe_geometry_handle == maybe_geometry_handle) {
          maybe_found_path = object_path;
        }
      }
      else {
        if (
          scene_object.maybe_transform_handle == object.maybe_transform_handle
        ) {
          maybe_found_path = object_path;
        }
      }
    },
    scene_handles,
    tree_paths
  );

  return maybe_found_path;
}


bool ObservedScene::clipboardContainsABody() const
{
  return clipboard.maybe_cut_body_index.hasValue();
}


bool ObservedScene::clipboardContainsAMarker() const
{
  return clipboard.maybe_cut_marker_index.hasValue();
}


BodyIndex
ObservedScene::pasteBodyGlobal(Optional<BodyIndex> maybe_new_parent_body_index)
{
  assert (clipboard.maybe_cut_body_index);
  BodyIndex old_body_index = *clipboard.maybe_cut_body_index;

  Optional<BodyIndex> maybe_old_parent_body_index =
    scene_state.body(old_body_index).maybe_parent_index;

  removeBodyBranchItemsFromTree(
    old_body_index, tree_widget, tree_paths, scene_state
  );

  removeBodyBranchObjectsFromScene(
    old_body_index, scene, scene_handles, scene_state
  );

  assert(maybe_new_parent_body_index != old_body_index);

  scene_state.body(old_body_index).maybe_parent_index =
    maybe_new_parent_body_index;

  BodyIndex new_body_index = old_body_index;

  changeBodyTransformToPreserveGlobal(
    new_body_index,
    scene_state,
    maybe_old_parent_body_index,
    maybe_new_parent_body_index
  );

  createBodyBranchItemsInTree(
    new_body_index, tree_widget, tree_paths, scene_state
  );

  createBodyBranchObjectsInScene(
    new_body_index, scene, scene_handles, scene_state
  );

  return new_body_index;
}


MarkerIndex
ObservedScene::pasteMarkerGlobal(
  Optional<BodyIndex>
    maybe_new_parent_body_index
)
{
  assert (clipboard.maybe_cut_marker_index);
  MarkerIndex marker_index = *clipboard.maybe_cut_marker_index;

  Optional<BodyIndex> maybe_old_parent_body_index =
    scene_state.marker(marker_index).maybe_body_index;

  removeMarkerItemFromTree(marker_index, tree_widget, tree_paths);
  removeMarkerObjectFromScene(marker_index, scene, scene_handles);

  scene_state.marker(marker_index).maybe_body_index =
    maybe_new_parent_body_index;

  changeMarkerPositionToPreserveGlobal(
    marker_index,
    scene_state,
    maybe_old_parent_body_index,
    maybe_new_parent_body_index
  );

  createMarkerItemInTree(
    marker_index, tree_widget, tree_paths, scene_state
  );

  createMarkerObjectInScene(
    marker_index, scene, scene_handles, scene_state
  );

  return marker_index;
}


void
ObservedScene::removingMarker(
  ObservedScene &observed_scene, MarkerIndex marker_index
)
{
  TreePaths &tree_paths = observed_scene.tree_paths;
  TreeWidget &tree_widget = observed_scene.tree_widget;
  SceneHandles &scene_handles = observed_scene.scene_handles;
  Scene &scene = observed_scene.scene;
  removeMarkerFromTree(marker_index, tree_paths, tree_widget);
  removeMarkerFromScene(scene, scene_handles, marker_index);
}


void
ObservedScene::removingBody(
  ObservedScene &observed_scene, BodyIndex body_index
)
{
  Scene &scene = observed_scene.scene;
  SceneHandles &scene_handles = observed_scene.scene_handles;
  SceneState &scene_state = observed_scene.scene_state;
  TreePaths &tree_paths = observed_scene.tree_paths;
  TreeWidget &tree_widget = observed_scene.tree_widget;
  removeBodyFromScene(scene, scene_handles, scene_state, body_index);
  removeBodyFromTree(tree_widget, tree_paths, scene_state, body_index);
}


void ObservedScene::removeBody(BodyIndex body_index)
{
  ObservedScene &observed_scene = *this;
  TreeWidget &tree_widget = observed_scene.tree_widget;
  TreePaths &tree_paths = observed_scene.tree_paths;
  SceneState &scene_state = observed_scene.scene_state;
  Scene &scene = observed_scene.scene;
  SceneHandles &scene_handles = observed_scene.scene_handles;

  clearClipboard(observed_scene);

  struct Visitor {
    ObservedScene &observed_scene;

    void visitMarker(MarkerIndex arg) const
    {
      ObservedScene::removingMarker(observed_scene, arg);
    }

    void visitBody(BodyIndex arg) const
    {
      ObservedScene::removingBody(observed_scene, arg);
    }
  };

  Visitor visitor{observed_scene};
  removeBodyFromSceneState(body_index, scene_state, visitor);
  updateTreeDistanceErrorMarkerOptions(tree_widget, tree_paths, scene_state);
  updateSceneObjects(scene, scene_handles, scene_state);
}


void ObservedScene::removeMarker(MarkerIndex marker_index)
{
  clearClipboard(*this);
  ObservedScene::removingMarker(*this, marker_index);
  scene_state.removeMarker(marker_index);
  updateSceneObjects(scene, scene_handles, scene_state);
}


void ObservedScene::removeBox(BodyIndex body_index, BoxIndex box_index)
{
  clearClipboard(*this);

  removeBoxFromTree(
    tree_widget, tree_paths, scene_state, body_index, box_index
  );

  removeBoxFromScene(scene, scene_handles, scene_state, body_index, box_index);
  removeIndexFrom(scene_state.body(body_index).boxes, box_index);
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


void ObservedScene::clearClipboard(ObservedScene &observed_scene)
{
  Clipboard &clipboard = observed_scene.clipboard;
  TreeWidget &tree_widget = observed_scene.tree_widget;
  TreePaths &tree_paths = observed_scene.tree_paths;

  if (clipboard.maybe_cut_body_index) {
    setBranchPending(
      tree_widget,
      tree_paths.body(*clipboard.maybe_cut_body_index).path,
      false
    );

    clipboard.maybe_cut_body_index.reset();
  }

  if (clipboard.maybe_cut_marker_index) {
    setBranchPending(
      tree_widget,
      tree_paths.marker(*clipboard.maybe_cut_marker_index).path,
      false
    );

    clipboard.maybe_cut_marker_index.reset();
  }
}


void ObservedScene::cutBody(BodyIndex body_index)
{
  ObservedScene &observed_scene = *this;
  Clipboard &clipboard = observed_scene.clipboard;
  TreeWidget &tree_widget = observed_scene.tree_widget;
  clearClipboard(observed_scene);

  setBranchPending(
    tree_widget, observed_scene.tree_paths.body(body_index).path, true
  );

  clipboard.maybe_cut_body_index = body_index;
}


void ObservedScene::cutMarker(MarkerIndex marker_index)
{
  ObservedScene &observed_scene = *this;
  Clipboard &clipboard = observed_scene.clipboard;
  TreeWidget &tree_widget = observed_scene.tree_widget;
  clearClipboard(observed_scene);

  setBranchPending(
    tree_widget, observed_scene.tree_paths.marker(marker_index).path, true
  );

  clipboard.maybe_cut_marker_index = marker_index;
}


void
ObservedScene::attachProperDraggerToSelectedObject(
  ObservedScene &observed_scene
)
{
  Scene &scene = observed_scene.scene;
  TreeWidget &tree_widget = observed_scene.tree_widget;
  TreePaths &tree_paths = observed_scene.tree_paths;

  Optional<GeometryHandle> selected_geometry = scene.selectedGeometry();

  if (selected_geometry) {
    if (scene.maybeLine(*selected_geometry)) {
      // There's no dragger for a line.
      return;
    }
  }
  else if (!scene.selectedTransform()) {
    return;
  }

  TreeItemDescription item =
    describePath(*tree_widget.selectedItem(), tree_paths);

  if (item.has_rotation_ancestor) {
    scene.attachDraggerToSelectedNode(Scene::DraggerType::rotate);
  }
  else if (item.maybe_box_index) {
    scene.attachDraggerToSelectedNode(Scene::DraggerType::scale);
  }
  else {
    scene.attachDraggerToSelectedNode(Scene::DraggerType::translate);
  }
}


void ObservedScene::handleTreeSelectionChanged()
{
  ObservedScene &observed_scene = *this;
  TreeWidget &tree_widget = observed_scene.tree_widget;
  Scene &scene = observed_scene.scene;
  Optional<TreePath> maybe_selected_item_path = tree_widget.selectedItem();

  if (!maybe_selected_item_path) {
    cerr << "No tree item selected\n";
    return;
  }

  SceneObject scene_object =
    sceneObjectForTreeItem(
      *maybe_selected_item_path,
      observed_scene.tree_paths,
      observed_scene.scene_handles
    );

  if (scene_object.maybe_geometry_handle) {
    scene.selectGeometry(*scene_object.maybe_geometry_handle);
    ObservedScene::attachProperDraggerToSelectedObject(observed_scene);
  }
  else if (scene_object.maybe_transform_handle) {
    scene.selectTransform(*scene_object.maybe_transform_handle);
    ObservedScene::attachProperDraggerToSelectedObject(observed_scene);
  }
  else {
    cerr << "No scene object found\n";
  }
}


void ObservedScene::handleSceneSelectionChanged()
{
  ObservedScene &observed_scene = *this;
  Scene &scene = observed_scene.scene;
  TreeWidget &tree_widget = observed_scene.tree_widget;

  Optional<GeometryHandle>
    maybe_selected_geometry = scene.selectedGeometry();

  if (!maybe_selected_geometry) {
    cerr << "No object selected in the scene.\n";
    return;
  }

  TransformHandle transform_handle =
    scene.parentTransform(*maybe_selected_geometry);

  SceneObject scene_object;
  scene_object.maybe_transform_handle = transform_handle;
  scene_object.maybe_geometry_handle = *maybe_selected_geometry;

  Optional<TreePath> maybe_tree_path =
    treeItemForSceneObject(
      scene_object,
      observed_scene.tree_paths,
      observed_scene.scene_handles
    );

  if (maybe_tree_path) {
    tree_widget.selectItem(*maybe_tree_path);
  }
  else {
    cerr << "No tree item for scene object.\n";
  }

  ObservedScene::attachProperDraggerToSelectedObject(observed_scene);
}


ObservedScene::ObservedScene(
  Scene &scene,
  TreeWidget &tree_widget,
  std::function<void(SceneState &)> update_errors_function
)
: scene(scene),
  tree_widget(tree_widget),
  scene_handles(createSceneObjects(scene_state, scene)),
  tree_paths(fillTree(tree_widget,scene_state)),
  update_errors_function(std::move(update_errors_function))
{
}


void
ObservedScene::createBodyInTree(
  BodyIndex body_index,
  ObservedScene &observed_scene
)
{
  TreeWidget &tree_widget = observed_scene.tree_widget;
  TreePaths &tree_paths = observed_scene.tree_paths;
  SceneState &scene_state = observed_scene.scene_state;
  ::createBodyInTree(tree_widget, tree_paths, scene_state, body_index);
}


void
ObservedScene::createBoxInTree(
  BodyIndex body_index,
  BoxIndex box_index,
  ObservedScene &observed_scene
)
{
  TreeWidget &tree_widget = observed_scene.tree_widget;
  TreePaths &tree_paths = observed_scene.tree_paths;
  SceneState &scene_state = observed_scene.scene_state;
  ::createBoxInTree(tree_widget, tree_paths, scene_state, body_index, box_index);
}


void
ObservedScene::createBodyInScene(
  BodyIndex body_index,
  ObservedScene &observed_scene
)
{
  SceneHandles &scene_handles = observed_scene.scene_handles;
  SceneState &scene_state = observed_scene.scene_state;
  Scene &scene = observed_scene.scene;
  ::createBodyInScene(scene, scene_handles, scene_state, body_index);
}


void
ObservedScene::createBoxInScene(
  BodyIndex body_index, BoxIndex box_index, ObservedScene &observed_scene
)
{
  Scene &scene = observed_scene.scene;
  SceneHandles &scene_handles = observed_scene.scene_handles;
  ::createBoxInScene(scene, scene_handles, body_index, box_index);
}


BodyIndex ObservedScene::addBody(Optional<BodyIndex> maybe_parent_body_index)
{
  ObservedScene &observed_scene = *this;
  SceneState &scene_state = observed_scene.scene_state;
  BodyIndex body_index = scene_state.createBody(maybe_parent_body_index);
  scene_state.body(body_index).addBox();
  ObservedScene::createBodyInScene(body_index, observed_scene);
  ObservedScene::createBodyInTree(body_index, observed_scene);

  return body_index;
}


BoxIndex ObservedScene::addBoxTo(BodyIndex body_index)
{
  BoxIndex box_index = scene_state.body(body_index).addBox();
  ObservedScene::createBoxInScene(body_index, box_index, *this);
  ObservedScene::createBoxInTree(body_index, box_index, *this);
  return box_index;
}


MarkerIndex ObservedScene::addMarker(Optional<BodyIndex> maybe_body_index)
{
  ObservedScene &observed_scene = *this;
  TreePaths &tree_paths = observed_scene.tree_paths;
  SceneState &scene_state = observed_scene.scene_state;
  TreeWidget &tree_widget = observed_scene.tree_widget;
  MarkerIndex marker_index = scene_state.createMarker(maybe_body_index);
  createMarkerInScene(marker_index, observed_scene);
  createMarkerInTree(marker_index, observed_scene);
  updateTreeDistanceErrorMarkerOptions(tree_widget, tree_paths, scene_state);
  return marker_index;
}


void
ObservedScene::createMarkerInScene(
  MarkerIndex marker_index, ObservedScene &observed_scene
)
{
  SceneHandles &scene_handles = observed_scene.scene_handles;
  SceneState &scene_state = observed_scene.scene_state;
  Scene &scene = observed_scene.scene;
  ::createMarkerInScene(scene, scene_handles, scene_state, marker_index);
}


void
ObservedScene::createMarkerInTree(
  MarkerIndex marker_index, ObservedScene &observed_scene
)
{
  TreeWidget &tree_widget = observed_scene.tree_widget;
  TreePaths &tree_paths = observed_scene.tree_paths;
  SceneState &scene_state = observed_scene.scene_state;
  ::createMarkerInTree(tree_widget, tree_paths, scene_state, marker_index);
}


bool
Clipboard::canPasteTo(
  Optional<BodyIndex> maybe_body_index,
  const SceneState &scene_state
) const
{
  if (maybe_cut_body_index) {
    if (maybe_body_index) {
      if (hasAncestor(*maybe_body_index, *maybe_cut_body_index, scene_state)) {
        return false;
      }
    }

    return true;
  }

  if (maybe_cut_marker_index) {
    return true;
  }

  return false;
}


bool ObservedScene::canPasteTo(Optional<BodyIndex> maybe_body_index)
{
  return clipboard.canPasteTo(maybe_body_index, scene_state);
}


void ObservedScene::replaceSceneStateWith(const SceneState &new_state)
{
  ObservedScene &observed_scene = *this;
  Scene &scene = observed_scene.scene;
  SceneHandles &scene_handles = observed_scene.scene_handles;
  TreeWidget &tree_widget = observed_scene.tree_widget;
  SceneState &scene_state = observed_scene.scene_state;
  destroySceneObjects(scene, scene_state, scene_handles);
  clearTree(tree_widget, observed_scene.tree_paths);
  clipboard.maybe_cut_body_index.reset();
  scene_state = new_state;
  scene_handles = createSceneObjects(scene_state, scene);
  observed_scene.tree_paths = fillTree(tree_widget, scene_state);
}


void ObservedScene::selectMarker(MarkerIndex marker_index)
{
  tree_widget.selectItem(tree_paths.marker(marker_index).path);
  handleTreeSelectionChanged();
}


void ObservedScene::selectBody(BodyIndex body_index)
{
  selectBodyInTree(body_index, *this);
  handleTreeSelectionChanged();
}


BodyIndex
ObservedScene::duplicateBody(
  BodyIndex body_index,
  MarkerNameMap &marker_name_map,
  ObservedScene &observed_scene
)
{
  SceneState &scene_state = observed_scene.scene_state;
  TaggedValue root_tag_value("");
  createBodyTaggedValue(root_tag_value, body_index, scene_state);

  BodyIndex new_body_index =
    createBodyFromTaggedValue(
      scene_state,
      root_tag_value.children[0],
      scene_state.body(body_index).maybe_parent_index,
      marker_name_map
    );

  ObservedScene::createBodyInTree(new_body_index, observed_scene);
  ObservedScene::createBodyInScene(new_body_index, observed_scene);

  return new_body_index;
}


BodyIndex ObservedScene::duplicateBody(BodyIndex body_index)
{
  MarkerNameMap marker_name_map;
  return ObservedScene::duplicateBody(body_index, marker_name_map, *this);
}


MarkerIndex ObservedScene::duplicateMarker(MarkerIndex marker_index)
{
  ObservedScene &observed_scene = *this;
  SceneState &scene_state = observed_scene.scene_state;
  MarkerIndex new_marker_index = scene_state.duplicateMarker(marker_index);
  ObservedScene::createMarkerInTree(new_marker_index, observed_scene);
  ObservedScene::createMarkerInScene(new_marker_index, observed_scene);
  return new_marker_index;
}


BodyIndex
ObservedScene::duplicateBodyWithDistanceErrors(BodyIndex body_index)
{
  MarkerNameMap marker_name_map;

  BodyIndex new_body_index =
    ObservedScene::duplicateBody(body_index, marker_name_map, *this);

  for (auto &map_entry : marker_name_map) {
    MarkerIndex marker1_index = *findMarkerIndex(scene_state, map_entry.first);
    MarkerIndex marker2_index = *findMarkerIndex(scene_state, map_entry.second);
    addDistanceError(marker1_index, marker2_index);
  }

  return new_body_index;
}


MarkerIndex
ObservedScene::duplicateMarkerWithDistanceError(MarkerIndex marker_index)
{
  MarkerIndex new_marker_index = duplicateMarker(marker_index);
  addDistanceError(marker_index, new_marker_index);
  return new_marker_index;
}


DistanceErrorIndex
ObservedScene::addDistanceError(
  Optional<MarkerIndex> optional_start_marker_index,
  Optional<MarkerIndex> optional_end_marker_index
)
{
  ObservedScene &observed_scene = *this;
  DistanceErrorIndex index = scene_state.createDistanceError();

  SceneState::DistanceError &distance_error =
    scene_state.distance_errors[index];

  scene_state.distance_errors[index].optional_start_marker_index =
    optional_start_marker_index;

  scene_state.distance_errors[index].optional_end_marker_index =
    optional_end_marker_index;

  observed_scene.update_errors_function(scene_state);

  createDistanceErrorInScene(scene, scene_handles, scene_state, index);

  createDistanceErrorInTree(
    distance_error,
    tree_widget,
    tree_paths,
    scene_state
  );

  updateSceneObjects(scene, scene_handles, scene_state);
  updateTreeValues(tree_widget, tree_paths, scene_state);
  return index;
}


void ObservedScene::selectDistanceError(DistanceErrorIndex index)
{
  tree_widget.selectItem(tree_paths.distance_errors[index].path);
}


void ObservedScene::selectBox(BodyIndex body_index, BoxIndex box_index)
{
  tree_widget.selectItem(tree_paths.body(body_index).boxes[box_index].path);
  handleTreeSelectionChanged();
}


void ObservedScene::handleSceneStateChanged()
{
  updateTreeValues(tree_widget, tree_paths, scene_state);
  updateSceneObjects(scene, scene_handles, scene_state);
}


auto
ObservedScene::describePath(const TreePath &path, const TreePaths &tree_paths)
-> TreeItemDescription
{
  TreeItemDescription description;
  using ItemType = TreeItemDescription::Type;

  if (path == tree_paths.path) {
    description.type = ItemType::scene;
    return description;
  }

  for (auto body_index : indicesOf(tree_paths.bodies)) {
    const TreePaths::Body &body_paths = tree_paths.body(body_index);

    if (startsWith(path, body_paths.translation.path)) {
      description.has_translation_ancesor = true;

      if (path == body_paths.translation.path) {
        description.type = ItemType::translation;
      }

      description.maybe_body_index = body_index;
      return description;
    }

    if (startsWith(path, body_paths.rotation.path)) {
      description.has_rotation_ancestor = true;
      description.maybe_body_index = body_index;

      if (path == body_paths.rotation.path) {
        description.type = ItemType::rotation;
      }

      return description;
    }

    size_t n_boxes = body_paths.boxes.size();

    for (size_t box_index = 0; box_index != n_boxes; ++box_index) {
      const TreePaths::Box &box_paths = body_paths.boxes[box_index];

      if (startsWith(path, box_paths.path)) {
        if (path == box_paths.path) {
          description.type = ItemType::box;
        }

        description.maybe_body_index = body_index;
        description.maybe_box_index = box_index;
        return description;
      }
    }

    if (body_paths.path == path) {
      description.type = ItemType::body;
      description.maybe_body_index = body_index;
      return description;
    }
  }

  for (auto i : indicesOf(tree_paths.markers)) {
    if (path == tree_paths.marker(i).path) {
      description.type = ItemType::marker;
      description.maybe_marker_index = i;
      return description;
    }
  }

  for (auto i : indicesOf(tree_paths.distance_errors)) {
    if (path == tree_paths.distance_errors[i].path) {
      description.type = ItemType::distance_error;
      description.maybe_distance_error_index = i;
      return description;
    }
  }

  return description;
}
