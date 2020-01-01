#include "observedscene.hpp"

#include "startswith.hpp"
#include "scenestatetaggedvalue.hpp"
#include "transformstate.hpp"
#include "globaltransform.hpp"
#include "treevalues.hpp"
#include "sceneobjects.hpp"

#define NEW_TRANSFER_TECHNIQUE 0

using TransformHandle = Scene::TransformHandle;
using std::cerr;


static bool isRotateItem(const TreePath &path, const TreePaths &tree_paths)
{
  for (auto i : indicesOf(tree_paths.bodies)) {
    if (startsWith(path, tree_paths.body(i).rotation.path)) {
      return true;
    }
  }

  return false;
}


static bool isScaleItem(const TreePath &path, const TreePaths &tree_paths)
{
  for (auto i : indicesOf(tree_paths.bodies)) {
    if (startsWith(path, tree_paths.body(i).geometry.path)) {
      return true;
    }
  }

  return false;
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
    f(scene_handles.marker(i).handle, tree_paths.marker(i).path);
  }

  for (auto i : indicesOf(tree_paths.bodies)) {
    f(scene_handles.body(i), tree_paths.body(i).path);
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


BodyIndex
ObservedScene::pasteGlobal(
  Optional<BodyIndex> maybe_new_parent_body_index,
  Clipboard &clipboard,
  ObservedScene &observed_scene
)
{
  SceneState &scene_state = observed_scene.scene_state;

#if !NEW_CUT_BEHAVIOR
  const TaggedValue &body_tagged_value =
    clipboard.clipboard_tagged_value.children[0];

  const Transform &old_parent_global_transform =
    clipboard.clipboard_transform;
#else
  BodyIndex old_body_index = *clipboard.maybe_cut_body_index;
#if !NEW_TRANSFER_TECHNIQUE
  clipboard.maybe_cut_body_index.reset();
  TaggedValue clipboard_tagged_value("clipboard");

  Optional<BodyIndex> maybe_old_parent_body_index =
    scene_state.body(old_body_index).maybe_parent_index;

  createBodyTaggedValue(clipboard_tagged_value, old_body_index, scene_state);

  Transform old_parent_global_transform =
    globalTransform(maybe_old_parent_body_index, scene_state);

  const TaggedValue &body_tagged_value = clipboard_tagged_value.children[0];

  ObservedScene::removeBody(observed_scene, old_body_index);

  if (maybe_new_parent_body_index) {
    if (*maybe_new_parent_body_index > old_body_index) {
      --*maybe_new_parent_body_index;
    }
  }
#else
  TreeWidget &tree_widget = observed_scene.tree_widget;
  TreePaths &tree_paths = observed_scene.tree_paths;
  removeBodyItemsFromTree(old_body_index, tree_widget, tree_paths, scene_state);
  removeBodyObjectsFromScene(old_body_index);
  scene_state.bodies[body_index].maybe_parent_index = maybe_new_parent_index;
#endif
#endif

  BodyIndex new_body_index =
    createBodyFromTaggedValue(
      scene_state,
      body_tagged_value,
      maybe_new_parent_body_index
    );

  SceneState::Body &body_state = scene_state.body(new_body_index);
  Transform old_body_transform = makeTransformFromState(body_state.transform);

  Transform body_global_transform =
    old_parent_global_transform*old_body_transform;

  Transform new_parent_global_transform =
    globalTransform(maybe_new_parent_body_index, scene_state);

  Transform new_body_transform =
    new_parent_global_transform.inverse()*body_global_transform;

  body_state.transform = transformState(new_body_transform);

#if !NEW_CUT_BEHAVIOR || !NEW_TRANSFER_TECHNIQUE
  ObservedScene::createBodyInTree(new_body_index, observed_scene);
  ObservedScene::createBodyInScene(new_body_index, observed_scene);
#else
  ObservedScene::createBodyItemsInTree(new_body_index, observed_scene);
  ObservedScene::createBodyObjectsInScene(new_body_index, observed_scene);
#endif
  return new_body_index;
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


void
ObservedScene::removeBody(
  ObservedScene &observed_scene, BodyIndex body_index
)
{
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

  removeBodyFromSceneState(
    body_index,
    observed_scene.scene_state,
    visitor
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
ObservedScene::cutBody(
  ObservedScene &observed_scene,
  BodyIndex body_index,
  Clipboard &clipboard
)
{
  TreeWidget &tree_widget = observed_scene.tree_widget;
  TreePaths &tree_paths = observed_scene.tree_paths;

#if !NEW_CUT_BEHAVIOR
  SceneState &scene_state = observed_scene.scene_state;

  Optional<BodyIndex> maybe_parent_body_index =
    scene_state.body(body_index).maybe_parent_index;

  clipboard.clipboard_tagged_value.children.clear();
  createBodyTaggedValue(clipboard.clipboard_tagged_value, body_index, scene_state);

  clipboard.clipboard_transform =
    globalTransform(maybe_parent_body_index, scene_state);

  ObservedScene::removeBody(observed_scene, body_index);
  updateTreeDistanceErrorMarkerOptions(tree_widget, tree_paths, scene_state);
#else
  if (clipboard.maybe_cut_body_index) {
    setBranchPending(
      tree_widget,
      tree_paths.body(*clipboard.maybe_cut_body_index).path,
      false
    );

    clipboard.maybe_cut_body_index.reset();
  }

  setBranchPending(
    tree_widget, observed_scene.tree_paths.body(body_index).path, true
  );

  clipboard.maybe_cut_body_index = body_index;
#endif
}


void
ObservedScene::attachProperDraggerToSelectedObject(
  ObservedScene &observed_scene
)
{
  Scene &scene = observed_scene.scene;
  TreeWidget &tree_widget = observed_scene.tree_widget;
  TreePaths &tree_paths = observed_scene.tree_paths;

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


void ObservedScene::handleTreeSelectionChanged(ObservedScene &observed_scene)
{
  TreeWidget &tree_widget = observed_scene.tree_widget;
  Scene &scene = observed_scene.scene;
  Optional<TreePath> maybe_selected_item_path = tree_widget.selectedItem();

  if (!maybe_selected_item_path) {
    cerr << "No tree item selected\n";
    return;
  }

  Optional<TransformHandle> maybe_object =
    sceneObjectForTreeItem(
      *maybe_selected_item_path,
      observed_scene.tree_paths,
      observed_scene.scene_handles
    );

  if (maybe_object) {
    scene.selectObject(*maybe_object);
    ObservedScene::attachProperDraggerToSelectedObject(observed_scene);
  }
  else {
    cerr << "No scene object found\n";
  }
}


void
ObservedScene::handleSceneSelectionChanged(
  ObservedScene &observed_scene
)
{
  Scene &scene = observed_scene.scene;
  TreeWidget &tree_widget = observed_scene.tree_widget;

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


ObservedScene::ObservedScene(Scene &scene, TreeWidget &tree_widget)
: scene(scene),
  tree_widget(tree_widget),
  scene_handles(createSceneObjects(scene_state, scene)),
  tree_paths(fillTree(tree_widget,scene_state))
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


BodyIndex
ObservedScene::addBody(
  Optional<BodyIndex> maybe_parent_body_index, ObservedScene &observed_scene
)
{
  SceneState &scene_state = observed_scene.scene_state;

  BodyIndex body_index =
    createBodyInState(scene_state, maybe_parent_body_index);

  ObservedScene::createBodyInScene(body_index, observed_scene);
  ObservedScene::createBodyInTree(body_index, observed_scene);

  return body_index;
}


MarkerIndex
ObservedScene::addMarker(
  ObservedScene &observed_scene,
  Optional<BodyIndex> maybe_body_index
)
{
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
