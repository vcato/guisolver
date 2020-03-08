#include "observedscene.hpp"

#include <sstream>
#include "startswith.hpp"
#include "scenestatetaggedvalue.hpp"
#include "treevalues.hpp"
#include "sceneobjects.hpp"
#include "sceneerror.hpp"
#include "scenestatetransform.hpp"
#include "removeindexfrom.hpp"
#include "vectorio.hpp"
#include "xyzcomponent.hpp"
#include "evaluateexpression.hpp"
#include "channel.hpp"
#include "meshstate.hpp"

using std::string;
using std::ostringstream;
using std::ostream;
using TransformHandle = Scene::TransformHandle;
using GeometryHandle = Scene::GeometryHandle;
using ManipulatorType = Scene::ManipulatorType;
using std::cerr;


template <typename XYZSolveFlags>
static MatchConst_t<bool, XYZSolveFlags> *
xyzSolveStateComponentPtr(
  XYZSolveFlags &xyz_solve_flags,
  XYZComponent component
)
{
  switch (component) {
    case XYZComponent::x: return &xyz_solve_flags.x;
    case XYZComponent::y: return &xyz_solve_flags.y;
    case XYZComponent::z: return &xyz_solve_flags.z;
  }

  return nullptr;
}


template <typename SceneState>
static MatchConst_t<bool, SceneState>*
channelSolveStatePtr(const Channel &channel, SceneState &)
{
  using SolveState = MatchConst_t<bool, SceneState>;
  SolveState *solve_state_ptr = nullptr;

  struct Visitor : Channel::Visitor {
    SolveState *&solve_state_ptr;

    Visitor(SolveState *&solve_state_ptr)
    : solve_state_ptr(solve_state_ptr)
    {
    }

    void visit(const BodyTranslationChannel &) const override
    {
      assert(false); // not implemented
      // solve_state_ptr = componentSolveStatePtr(channel);
    }

    void visit(const BodyRotationChannel &) const override
    {
      assert(false); // not implemented
    }

    void visit(const BodyScaleChannel &) const override
    {
      assert(false); // not implemented
    }

    void visit(const BodyBoxScaleChannel &) const override
    {
      assert(false); // not implemented
    }

    void visit(const BodyBoxCenterChannel &) const override
    {
      assert(false); // not implemented
    }

    void visit(const MarkerPositionChannel &) const override
    {
      assert(false); // not implemented
    }
  } visitor{solve_state_ptr};

  channel.accept(visitor);
  return solve_state_ptr;
}


template <typename SceneState>
static auto *
solveStatePtr(
  const BodyTranslationComponent &element,
  SceneState &scene_state
)
{
  auto &body_state = scene_state.body(element.body_index);
  auto &body_solve_flags = body_state.solve_flags;
  auto &xyz_solve_flags = body_solve_flags.translation;
  return xyzSolveStateComponentPtr(xyz_solve_flags, element.component);
}


template <typename SceneState>
static auto *
solveStatePtr(
  const BodyRotationComponent &element,
  SceneState &scene_state
)
{
  auto &body_state = scene_state.body(element.body_index);
  auto &body_solve_flags = body_state.solve_flags;
  auto &xyz_solve_flags = body_solve_flags.rotation;
  return xyzSolveStateComponentPtr(xyz_solve_flags, element.component);
}


template <typename SceneState>
static auto *
solveStatePtr(
  const BodyScale &element,
  SceneState &scene_state
)
{
  auto &body_state = scene_state.body(element.body_index);
  return &body_state.solve_flags.scale;
}


template <typename SceneState>
static auto*
basicPathSolveStatePtr(
  SceneState &scene_state,
  const TreePath &path,
  const TreePaths &tree_paths
)
{
  using SolveStatePtr = MatchConst_t<bool,SceneState> *;
  SolveStatePtr solve_state_ptr = nullptr;

  auto f = [&](const auto &element){
    solve_state_ptr = solveStatePtr(element, scene_state);
  };

  forSolvableSceneElement2(path, tree_paths, f);
  return solve_state_ptr;
}


static const TreePath &
solvePath(const TreePath &path, const TreePaths &tree_paths)
{
  const TreePath *tree_path_ptr = nullptr;

  struct BodyValueVisitor {
    const TreePaths::Body &body_paths;
    const TreePath *&tree_path_ptr;

    void visitTranslationComponent(XYZComponent component)
    {
      tree_path_ptr =
        &*body_paths.translation.component(component).maybe_solve_path;
    }

    void visitRotationComponent(XYZComponent component)
    {
      tree_path_ptr =
        &*body_paths.rotation.component(component).maybe_solve_path;
    }

    void visitScale()
    {
      tree_path_ptr = &*body_paths.scale.maybe_solve_path;
    }
  };

  struct ValueVisitor : SolvableSceneElementVisitor {
    const TreePaths &tree_paths;
    const TreePath *&tree_path_ptr;

    ValueVisitor(
      const TreePaths &tree_paths,
      const TreePath *&tree_path_ptr
    )
    : tree_paths(tree_paths),
      tree_path_ptr(tree_path_ptr)
    {
    }

    void visit(const BodyTranslationComponent &element) const override
    {
      auto &body_paths = tree_paths.body(element.body_index);
      BodyValueVisitor body_visitor{body_paths, tree_path_ptr};
      body_visitor.visitTranslationComponent(element.component);
    }

    void visit(const BodyRotationComponent &element) const override
    {
      auto &body_paths = tree_paths.body(element.body_index);
      BodyValueVisitor body_visitor{body_paths, tree_path_ptr};
      body_visitor.visitRotationComponent(element.component);
    }

    void visit(const BodyScale &element) const override
    {
      auto &body_paths = tree_paths.body(element.body_index);
      BodyValueVisitor body_visitor{body_paths, tree_path_ptr};
      body_visitor.visitScale();
    }
  };

  ValueVisitor value_visitor(tree_paths, tree_path_ptr);
  forSolvableSceneElement(path, tree_paths, value_visitor);
  assert(tree_path_ptr);
  return *tree_path_ptr;
}


static bool*
pathSolveStatePtr(
  SceneState &scene_state,
  const TreePath &path,
  const TreePaths &tree_paths
)
{
  return basicPathSolveStatePtr(scene_state, path, tree_paths);
}


static const bool*
pathSolveStatePtr(
  const SceneState &scene_state,
  const TreePath &path,
  const TreePaths &tree_paths
)
{
  return basicPathSolveStatePtr(scene_state, path, tree_paths);
}


template <typename Index, typename Function>
static void
removeIndices(vector<Index> &indices, const Function &remove_function)
{
  size_t n = indices.size();

  for (size_t i=0; i!=n; ++i) {
    remove_function(indices[i]);

    for (size_t j=i+1; j!=n; ++j) {
      if (indices[j] > indices[i]) {
        --indices[j];
      }
    }
  }
}


static SceneTreeRef sceneTree(ObservedScene &observed_scene)
{
  return {observed_scene.tree_widget, observed_scene.tree_paths};
}


template <typename Visitor>
static void
removeBodyFromSceneState(
  BodyIndex body_index,
  SceneState &scene_state,
  const Visitor &visitor
)
{
  vector<BodyIndex> body_indices_to_remove;
  postOrderTraverseBodyBranch(body_index, scene_state, body_indices_to_remove);

  removeIndices(body_indices_to_remove, [&](BodyIndex i){
    removeMarkersOnBody(i, scene_state, visitor);
    visitor.visitBody(i);
    scene_state.removeBody(i);
  });
}


template <typename Visitor>
static void
removeMarkersOnBody(
  BodyIndex body_index,
  SceneState &scene_state,
  const Visitor &visitor
)
{
  vector<MarkerIndex> indices_of_markers_to_remove =
    markersOnBody(body_index, scene_state);

  removeIndices(indices_of_markers_to_remove, [&](MarkerIndex i){
    visitor.visitMarker(i);
    scene_state.removeMarker(i);
  });
}


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


namespace {
template <typename F>
struct GeometryForParams {
  const SceneHandles::Body &body_handles;
  const TreePaths::Body &body_paths;
  const F &f;
};
}


namespace {
template <typename F>
struct GeometryFor : GeometryVisitor, GeometryForParams<F> {
  using Params = GeometryForParams<F>;
  GeometryFor(Params params) : Params(params) {}

  void visitBox(BoxIndex box_index)
  {
    this->f(
      {
        this->body_handles.transformHandle(),
        this->body_handles.boxes[box_index].handle,
      },
      this->body_paths.boxes[box_index].path
    );
  }

  void visitLine(LineIndex line_index)
  {
    this->f(
      {
        this->body_handles.transformHandle(),
        this->body_handles.lines[line_index].handle,
      },
      this->body_paths.lines[line_index].path
    );
  }

  void visitMesh(MeshIndex mesh_index)
  {
    this->f(
      {
        this->body_handles.transformHandle(),
        this->body_handles.meshes[mesh_index].handle,
      },
      this->body_paths.meshes[mesh_index].path
    );
  }
};
}


template <typename F>
static void
forEachSceneObjectPath(
  const F &f,
  const SceneHandles &scene_handles,
  const TreePaths &tree_paths,
  const SceneState &scene_state
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

    f(
      {
        body_handles.transformHandle(),
        Optional<GeometryHandle>{},
      },
      body_paths.path
    );

    GeometryFor<F> geometry_for({body_handles, body_paths, f});
    forEachBodyGeometry(scene_state.body(body_index), geometry_for);
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
  const SceneHandles &scene_handles,
  const SceneState &scene_state
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
    tree_paths,
    scene_state
  );

  return scene_object;
}


static Optional<TreePath>
treeItemForSceneObject(
  const SceneObject &scene_object,
  const TreePaths &tree_paths,
  const SceneHandles &scene_handles,
  const SceneState &scene_state
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
    tree_paths,
    scene_state
  );

  return maybe_found_path;
}


struct ObservedScene::Impl {
  static void evaluateExpressions(ObservedScene &);
  static void evaluateChannelExpression(const Channel &, ObservedScene &);

  static TreeItemDescription
    describePath(
      const TreePath &path,
      const TreePaths &tree_paths,
      const SceneState &scene_state
    );

  static void
    setChannelExpression(
      const Channel &,
      const Expression &,
      ObservedScene &
    );
};


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
    old_body_index, sceneTree(*this), scene_state
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
    new_body_index, sceneTree(*this), scene_state
  );

  createBodyBranchObjectsInScene(
    new_body_index, scene, scene_handles, scene_state
  );

  return new_body_index;
}


MarkerIndex
ObservedScene::pasteMarkerGlobal(
  Optional<BodyIndex> maybe_new_parent_body_index
)
{
  assert (clipboard.maybe_cut_marker_index);
  MarkerIndex marker_index = *clipboard.maybe_cut_marker_index;

  Optional<BodyIndex> maybe_old_parent_body_index =
    scene_state.marker(marker_index).maybe_body_index;

  removeMarkerItemFromTree(marker_index, sceneTree(*this));
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
    marker_index, sceneTree(*this), scene_state
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
  SceneHandles &scene_handles = observed_scene.scene_handles;
  Scene &scene = observed_scene.scene;
  removeMarkerFromTree(marker_index, sceneTree(observed_scene));
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
  removeBodyFromScene(scene, scene_handles, scene_state, body_index);
  removeBodyFromTree(sceneTree(observed_scene), scene_state, body_index);
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
  updateTreeDistanceErrorMarkerOptions(tree_widget, tree_paths, scene_state);
}


void ObservedScene::removeBox(BodyIndex body_index, BoxIndex box_index)
{
  clearClipboard(*this);

  removeBoxFromTree(
    sceneTree(*this), scene_state, body_index, box_index
  );

  removeBoxFromScene(
    scene, scene_handles, scene_state, body_index, box_index
  );

  removeIndexFrom(scene_state.body(body_index).boxes, box_index);
}


void ObservedScene::removeLine(BodyIndex body_index, LineIndex line_index)
{
  clearClipboard(*this);

  removeLineFromTree(
    sceneTree(*this), scene_state, body_index, line_index
  );

  removeLineFromScene(
    scene, scene_handles, scene_state, body_index, line_index
  );

  removeIndexFrom(scene_state.body(body_index).lines, line_index);
}


void ObservedScene::removeMesh(BodyIndex body_index, MeshIndex mesh_index)
{
  clearClipboard(*this);

  removeMeshFromTree(
    sceneTree(*this), scene_state, body_index, mesh_index
  );

  removeMeshFromScene(
    scene, scene_handles, scene_state, body_index, mesh_index
  );

  removeIndexFrom(scene_state.body(body_index).meshes, mesh_index);
}


void ObservedScene::removeDistanceError(DistanceErrorIndex distance_error_index)
{
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
  solveScene();
  handleSceneStateChanged();
}


void ObservedScene::removeVariable(VariableIndex variable_index)
{
  removeVariableFromTree(variable_index, {tree_widget, tree_paths});
  removeVariableFromScene(scene, scene_handles, variable_index);
  scene_state.removeVariable(variable_index);

  // Not evaluating expressions here because removing a variable will just
  // cause any expressions that are using the variable to become invalid,
  // which doesn't change the value of the channel in which the expression is
  // used.
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


Optional<ManipulatorType>
ObservedScene::properManpiulatorForSelectedObject() const
{
  const ObservedScene &observed_scene = *this;
  Scene &scene = observed_scene.scene;
  TreeWidget &tree_widget = observed_scene.tree_widget;

  Optional<GeometryHandle> selected_geometry = scene.selectedGeometry();

  if (selected_geometry) {
    if (scene.maybeLine(*selected_geometry)) {
      // There's no dragger for a line.
      return {};
    }
  }
  else if (!scene.selectedTransform()) {
    return {};
  }

  ObservedScene::TreeItemDescription item =
    observed_scene.describePath(*tree_widget.selectedItem());

  if (item.has_rotation_ancestor) {
    return ManipulatorType::rotate;
  }
  else if (item.maybe_box_index) {
    return ManipulatorType::scale;
  }
  else if (item.maybe_mesh_index) {
    return ManipulatorType::scale;
  }
  else {
    return ManipulatorType::translate;
  }
}


void
ObservedScene::attachProperDraggerToSelectedObject(
  ObservedScene &observed_scene
)
{
  Optional<ManipulatorType> maybe_manipulator_type =
    observed_scene.properManpiulatorForSelectedObject();

  if (!maybe_manipulator_type) {
    return;
  }

  Scene &scene = observed_scene.scene;

  scene.attachManipulatorToSelectedNode(*maybe_manipulator_type);
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
      tree_paths,
      scene_handles,
      scene_state
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
    cerr << "handleTreeSelectionChanged: No scene object found\n";
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
      tree_paths,
      scene_handles,
      scene_state
    );

  if (maybe_tree_path) {
    tree_widget.selectItem(*maybe_tree_path);
  }
  else {
    cerr << "No tree item for scene object.\n";
  }

  ObservedScene::attachProperDraggerToSelectedObject(observed_scene);
}


#if 0
static string componentName(const XYZComponent &component)
{
  switch (component) {
    case XYZComponent::x: return "x";
    case XYZComponent::y: return "y";
    case XYZComponent::z: return "z";
  }

  assert(false);
  return "";
}
#endif


#if 0
static string channelName(const Channel &channel)
{
  string name;

  struct Visitor : Channel::Visitor {
    ostream &stream;

    Visitor(ostream &stream)
    : stream(stream)
    {
    }

    void visit(const BodyTranslationChannel &channel) const override
    {
      stream << "body(" << channel.body_index << ").transform.translation";
      stream << "." << componentName(channel.component);
    }

    void visit(const BodyRotationChannel &channel) const override
    {
      stream << "body(" << channel.body_index << ").transform.rotation";
      stream << "." << componentName(channel.component);
    }

    void visit(const BodyBoxScaleChannel &channel) const override
    {
      stream << "body(" << channel.body_index << ")";
      stream << ".box(" << channel.box_index << ")";
      stream << ".scale";
      stream << "." << componentName(channel.component);
    }

    void visit(const BodyBoxCenterChannel &channel) const override
    {
      stream << "body(" << channel.body_index << ")";
      stream << ".box(" << channel.box_index << ")";
      stream << ".center";
      stream << "." << componentName(channel.component);
    }

    void visit(const MarkerPositionChannel &channel) const override
    {
      stream << "marker(" << channel.marker_index << ")";
      stream << ".position";
      stream << "." << componentName(channel.component);
    }
  };

  ostringstream name_stream;
  channel.accept(Visitor(name_stream));
  return name_stream.str();
}
#endif


static void
setChannelExpression(
  const Channel &channel,
  const Expression &expression,
  SceneState &scene_state
)
{
  channelExpression(channel, scene_state) = expression;
}


template <typename SceneState>
static MatchConst_t<NumericValue, SceneState> &
channelValue(
  const Channel &channel,
  SceneState &scene_state
)
{
  using ValuePtr = MatchConst_t<NumericValue, SceneState> *;
  ValuePtr value_ptr = nullptr;

  struct Visitor : Channel::Visitor {
    SceneState &scene_state;
    ValuePtr &value_ptr;

    Visitor(
      SceneState &scene_state,
      ValuePtr &value_ptr
    )
    : scene_state(scene_state),
      value_ptr(value_ptr)
    {
    }

    void visit(const BodyTranslationChannel &channel) const override
    {
      value_ptr = &
        scene_state
          .body(channel.body_index)
          .transform
          .translation
          .component(channel.component);
    }

    void visit(const BodyRotationChannel &channel) const override
    {
      value_ptr = &
        scene_state
          .body(channel.body_index)
          .transform
          .rotation
          .component(channel.component);
    }

    void visit(const BodyScaleChannel &channel) const override
    {
      value_ptr = &
        scene_state
          .body(channel.body_index)
          .transform
          .scale;
    }

    void visit(const BodyBoxScaleChannel &channel) const override
    {
      value_ptr = &
        scene_state
          .body(channel.body_index)
          .boxes[channel.box_index]
          .scale
          .component(channel.component);
    }

    void visit(const BodyBoxCenterChannel &channel) const override
    {
      value_ptr = &
        scene_state
          .body(channel.body_index)
          .boxes[channel.box_index]
          .center
          .component(channel.component);
    }

    void visit(const MarkerPositionChannel &channel) const override
    {
      value_ptr = &
        scene_state
          .marker(channel.marker_index)
          .position
          .component(channel.component);
    }
  };

  channel.accept(Visitor{scene_state, value_ptr});
  assert(value_ptr);
  return *value_ptr;
}


static void
updateChannelTreeValue(
  const Channel &channel,
  TreeWidget &tree_widget,
  const TreePaths &tree_paths,
  const SceneState &scene_state
)
{
  const TreePath &path = channelPath(channel, tree_paths);
  NumericValue value = channelValue(channel, scene_state);
  tree_widget.setItemNumericValue(path, value);
}


template <typename F>
static void
forEachChannel(const SceneState &scene_state, const F &f)
{
  for (BodyIndex body_index : indicesOf(scene_state.bodies())) {
    f(BodyTranslationChannel{body_index, XYZComponent::x});
    f(BodyTranslationChannel{body_index, XYZComponent::y});
    f(BodyTranslationChannel{body_index, XYZComponent::z});
    f(BodyRotationChannel{body_index, XYZComponent::x});
    f(BodyRotationChannel{body_index, XYZComponent::y});
    f(BodyRotationChannel{body_index, XYZComponent::z});
    f(BodyScaleChannel(BodyScale{body_index}));
    BoxIndex n_boxes = scene_state.bodies()[body_index].boxes.size();

    for (BoxIndex box_index = 0; box_index != n_boxes; ++box_index) {
      f(BodyBoxScaleChannel{body_index, box_index, XYZComponent::x});
      f(BodyBoxScaleChannel{body_index, box_index, XYZComponent::y});
      f(BodyBoxScaleChannel{body_index, box_index, XYZComponent::z});
      f(BodyBoxCenterChannel{body_index, box_index, XYZComponent::x});
      f(BodyBoxCenterChannel{body_index, box_index, XYZComponent::y});
      f(BodyBoxCenterChannel{body_index, box_index, XYZComponent::z});
    }
  }

  for (MarkerIndex marker_index : indicesOf(scene_state.markers())) {
    f(MarkerPositionChannel{marker_index, XYZComponent::x});
    f(MarkerPositionChannel{marker_index, XYZComponent::y});
    f(MarkerPositionChannel{marker_index, XYZComponent::z});
  }
}


bool
forPathChannel(
  const TreePath &path,
  const TreePaths &tree_paths,
  const SceneState &scene_state,
  const std::function<void(const Channel &)> &channel_function
)
{
  bool found = false;

  forEachChannel(scene_state, [&](const Channel &channel){
    if (channelPath(channel, tree_paths) == path) {
      channel_function(channel);
      found = true;
    }
  });

  return found;
}


bool ObservedScene::pathSupportsExpressions(const TreePath &path) const
{
  bool path_is_channel =
    forPathChannel(path, tree_paths, scene_state,
      [&](const Channel &){}
    );

  return path_is_channel;
}


void
ObservedScene::Impl::setChannelExpression(
  const Channel &channel,
  const Expression &expression,
  ObservedScene &observed_scene
)
{
  SceneState &scene_state = observed_scene.scene_state;
  const TreePaths &tree_paths = observed_scene.tree_paths;
  const TreePath &path = channelPath(channel, tree_paths);
  TreeWidget &tree_widget = observed_scene.tree_widget;
  ::setChannelExpression(channel, expression, scene_state);
  evaluateChannelExpression(channel, observed_scene);
  bool *solve_state_ptr = observed_scene.solveStatePtr(path);

  if (solve_state_ptr) {
    bool new_solve_state = false;
    TreePath solve_path = solvePath(path, tree_paths);
    *solve_state_ptr = new_solve_state;
    setTreeBoolValue(tree_widget, solve_path, new_solve_state);
  }

  observed_scene.solveScene();
  observed_scene.handleSceneStateChanged();
}


void
ObservedScene::handleTreeExpressionChanged(
  const TreePath &path, const std::string &expression
)
{
  bool path_was_channel =
    forPathChannel(path, tree_paths, scene_state,
      [&](const Channel &channel){
        Impl::setChannelExpression(channel, expression, *this);
      }
    );

  if (!path_was_channel) {
    if (expression != "" ) {
      cerr << "setSceneStateExpression: path is not a channel "
        "path=" << path << ", "
        "expression=" << expression << "\n";

      return;
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


void
ObservedScene::handleTreeEnumerationIndexChanged(
  const TreePath &path, int value
)
{
  setSceneStateEnumerationIndex(scene_state, path, value, tree_paths);
  solveScene();
  handleSceneStateChanged();
}


ObservedScene::ObservedScene(
  Scene &scene,
  TreeWidget &tree_widget,
  std::function<void(SceneState &)> update_errors_function,
  std::function<void(SceneState &)> solve_function
)
: scene(scene),
  tree_widget(tree_widget),
  scene_handles(createSceneObjects(scene_state, scene)),
  tree_paths(fillTree(tree_widget,scene_state)),
  update_errors_function(std::move(update_errors_function)),
  solve_function(std::move(solve_function))
{
}


void
ObservedScene::createBodyInTree(
  BodyIndex body_index,
  ObservedScene &observed_scene
)
{
  SceneState &scene_state = observed_scene.scene_state;
  ::createBodyInTree(body_index, sceneTree(observed_scene), scene_state);
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


BodyIndex ObservedScene::addBody(Optional<BodyIndex> maybe_parent_body_index)
{
  ObservedScene &observed_scene = *this;
  SceneState &scene_state = observed_scene.scene_state;
  BodyIndex body_index = scene_state.createBody(maybe_parent_body_index);
  scene_state.body(body_index).createBox();
  ObservedScene::createBodyInScene(body_index, observed_scene);
  ObservedScene::createBodyInTree(body_index, observed_scene);

  return body_index;
}


BoxIndex ObservedScene::addBoxTo(BodyIndex body_index)
{
  BoxIndex box_index = scene_state.body(body_index).createBox();
  ::createBoxInScene(scene, scene_handles, body_index, box_index);

  ::createBoxInTree(
    sceneTree(*this), scene_state, body_index, box_index
  );

  return box_index;
}


BoxIndex ObservedScene::addLineTo(BodyIndex body_index)
{
  LineIndex line_index = scene_state.body(body_index).createLine();

  ::createLineInScene(
    scene, scene_handles, body_index, line_index, scene_state
  );

  ::createLineInTree(
    sceneTree(*this), scene_state, body_index, line_index
  );

  return line_index;
}


MeshIndex ObservedScene::addMeshTo(BodyIndex body_index, const Mesh &mesh)
{
  MeshIndex mesh_index =
    scene_state.body(body_index).createMesh(meshShapeStateFromMesh(mesh));

  ::createMeshInScene(
    scene, scene_handles, body_index, mesh_index, scene_state
  );

  ::createMeshInTree(
    sceneTree(*this), scene_state, body_index, mesh_index
  );

  return mesh_index;
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
  SceneState &scene_state = observed_scene.scene_state;
  ::createMarkerInTree(marker_index, sceneTree(observed_scene), scene_state);
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
  destroySceneObjects(scene, scene_state, scene_handles);
  clearTree(tree_widget, tree_paths);
  clipboard.maybe_cut_body_index.reset();
  scene_state = new_state;
  scene_handles = createSceneObjects(scene_state, scene);
  tree_paths = fillTree(tree_widget, scene_state);
}


void ObservedScene::solveScene()
{
  solve_function(scene_state);
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


MarkerIndex ObservedScene::duplicateMarker(MarkerIndex source_marker_index)
{
  MarkerIndex new_marker_index =
    scene_state.duplicateMarker(source_marker_index);

  createMarkerInTree(new_marker_index, *this);
  createMarkerInScene(new_marker_index, *this);

  updateTreeDistanceErrorMarkerOptions(tree_widget, tree_paths, scene_state);
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
    addDistanceError(marker1_index, marker2_index, /*body*/{});
  }

  return new_body_index;
}


MarkerIndex
ObservedScene::duplicateMarkerWithDistanceError(MarkerIndex marker_index)
{
  MarkerIndex new_marker_index = duplicateMarker(marker_index);

  Optional<BodyIndex> maybe_body_index =
    scene_state.marker(marker_index).maybe_body_index;

  addDistanceError(marker_index, new_marker_index, maybe_body_index);
  return new_marker_index;
}


DistanceErrorIndex
ObservedScene::addDistanceError(
  Optional<MarkerIndex> optional_start_marker_index,
  Optional<MarkerIndex> optional_end_marker_index,
  Optional<BodyIndex> optional_body_index
)
{
  ObservedScene &observed_scene = *this;

  DistanceErrorIndex index =
    scene_state.createDistanceError(optional_body_index);

  SceneState::DistanceError &distance_error =
    scene_state.distance_errors[index];

  distance_error.optional_start_marker_index = optional_start_marker_index;
  distance_error.optional_end_marker_index = optional_end_marker_index;
  observed_scene.update_errors_function(scene_state);
  createDistanceErrorInScene(scene, scene_handles, scene_state, index);

  createDistanceErrorInTree(
    index,
    { tree_widget, tree_paths },
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


void ObservedScene::selectLine(BodyIndex body_index, LineIndex line_index)
{
  tree_widget.selectItem(tree_paths.body(body_index).lines[line_index].path);
  handleTreeSelectionChanged();
}


void ObservedScene::selectMesh(BodyIndex body_index, MeshIndex mesh_index)
{
  tree_widget.selectItem(tree_paths.body(body_index).meshes[mesh_index].path);
  handleTreeSelectionChanged();
}


const bool *ObservedScene::solveStatePtr(const TreePath &path) const
{
  return ::pathSolveStatePtr(scene_state, path, tree_paths);
}


bool *ObservedScene::solveStatePtr(const TreePath &path)
{
  return ::pathSolveStatePtr(scene_state, path, tree_paths);
}


void ObservedScene::handleSceneStateChanged()
{
  updateTreeValues(tree_widget, tree_paths, scene_state);
  updateSceneObjects(scene, scene_handles, scene_state);
}


namespace {
struct DescribeGeometryParams {
  using TreeItemDescription = ObservedScene::TreeItemDescription;

  bool &found_description;
  const TreePaths::Body &body_paths;
  const TreePath &path;
  TreeItemDescription &description;
  const BodyIndex body_index;
};
}


namespace {
struct DescribeGeometry : GeometryVisitor, DescribeGeometryParams {
  using Params = DescribeGeometryParams;
  using TreeItemDescription = ObservedScene::TreeItemDescription;
  using ItemType = TreeItemDescription::Type;

  DescribeGeometry(DescribeGeometryParams params) : Params(params) {}

  void visitBox(BoxIndex box_index)
  {
    if (!found_description) {
      const TreePaths::Box &box_paths = body_paths.boxes[box_index];

      if (startsWith(path, box_paths.path)) {
        if (path == box_paths.path) {
          description.type = ItemType::box;
        }

        description.maybe_box_index = box_index;
        found_description = true;
      }
    }
  }

  void visitLine(LineIndex line_index)
  {
    if (!found_description) {
      const TreePaths::Line &line_paths = body_paths.lines[line_index];

      if (startsWith(path, line_paths.path)) {
        if (path == line_paths.path) {
          description.type = ItemType::line;
        }

        description.maybe_line_index = line_index;
        found_description = true;
      }
    }
  }

  void visitMesh(MeshIndex mesh_index)
  {
    if (!found_description) {
      const TreePaths::Mesh &mesh_paths = body_paths.meshes[mesh_index];

      if (startsWith(path, mesh_paths.path)) {
        if (path == mesh_paths.path) {
          description.type = ItemType::mesh;
        }

        description.maybe_mesh_index = mesh_index;
        found_description = true;
      }
    }
  }
};
}


auto
ObservedScene::Impl::describePath(
  const TreePath &path,
  const TreePaths &tree_paths,
  const SceneState &scene_state
)
-> TreeItemDescription
{
  TreeItemDescription description;
  using ItemType = TreeItemDescription::Type;

  if (path == tree_paths.path) {
    description.type = ItemType::scene;
    return description;
  }

  for (BodyIndex body_index : indicesOf(tree_paths.bodies)) {
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

    {
      bool found_description = false;

      DescribeGeometry
        describe_geometry({
          found_description,
            body_paths,
            path,
            description,
            body_index
        });

      forEachBodyGeometry(scene_state.body(body_index), describe_geometry);

      if (found_description) {
        description.maybe_body_index = body_index;
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

  for (auto i : indicesOf(tree_paths.variables)) {
    if (path == tree_paths.variables[i].path) {
      description.type = ItemType::variable;
      description.maybe_variable_index = i;
      return description;
    }
  }

  return description;
}


VariableIndex ObservedScene::addVariable()
{
  VariableIndex index = scene_state.createVariable();
  createVariableInTree(index, sceneTree(*this), scene_state);
  return index;
}


void ObservedScene::selectMarker(MarkerIndex marker_index)
{
  tree_widget.selectItem(tree_paths.marker(marker_index).path);
  handleTreeSelectionChanged();
}


void ObservedScene::selectVariable(VariableIndex variable_index)
{
  tree_widget.selectItem(tree_paths.variables[variable_index].path);
  handleTreeSelectionChanged();
}


static void
evaluateExpression(
  const Channel &channel,
  EvaluationEnvironment &environment,
  SceneState &scene_state
)
{
  Expression &expression = channelExpression(channel, scene_state);

  Optional<NumericValue> maybe_result =
    evaluateExpression(expression, /*error_stream*/cerr, environment);

  if (!maybe_result) {
    return;
  }
  else {
    channelValue(channel, scene_state) = *maybe_result;
  }
}


void
ObservedScene::Impl::evaluateChannelExpression(
  const Channel &channel,
  ObservedScene &observed_scene
)
{
  SceneState &scene_state = observed_scene.scene_state;
  TreeWidget &tree_widget = observed_scene.tree_widget;
  const TreePaths &tree_paths = observed_scene.tree_paths;
  EvaluationEnvironment environment;

  for (VariableIndex variable_index : indicesOf(scene_state.variables)) {
    const SceneState::Variable &variable =
      scene_state.variables[variable_index];

    environment[variable.name] = variable.value;
  }

  Expression &expression = channelExpression(channel, scene_state);

  if (!expression.empty()) {
    evaluateExpression(
      channel,
      environment,
      scene_state
    );

    updateChannelTreeValue(channel, tree_widget, tree_paths, scene_state);
  }
}


void
ObservedScene::Impl::evaluateExpressions(ObservedScene &observed_scene)
{
  SceneState &scene_state = observed_scene.scene_state;

  forEachChannel(scene_state, [&](const Channel& channel){
    evaluateChannelExpression(channel, observed_scene);
  });
}


void
ObservedScene::handleTreeNumericValueChanged(
  const TreePath &path,
  NumericValue value
)
{
  ObservedScene &observed_scene = *this;
  const TreePaths &tree_paths = observed_scene.tree_paths;
  SceneState &state = observed_scene.scene_state;

  bool value_was_changed =
    setSceneStateNumericValue(state, path, value, tree_paths);

  if (value_was_changed) {
    Impl::evaluateExpressions(*this);

    {
      bool *solve_state_ptr = ::pathSolveStatePtr(state, path, tree_paths);

      // Turn off the solve state of the value that is being changed, so that
      // it doesn't give strange feedback to the user.

      if (solve_state_ptr) {
        *solve_state_ptr = false;
      }

      observed_scene.solveScene();

      if (solve_state_ptr) {
        TreePath solve_path = solvePath(path, tree_paths);
        setTreeBoolValue(tree_widget, solve_path, false);
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
ObservedScene::handleTreeStringValueChanged(
  const TreePath &path,
  const StringValue &value
)
{
  bool value_was_changed =
    setSceneStateStringValue(scene_state, path, value, tree_paths);

  if (!value_was_changed) {
    forEachChannel(scene_state, [&](const Channel &channel){
      const TreePath *expression_path_ptr =
        channelExpressionPathPtr(channel, tree_paths);

      if (expression_path_ptr) {
        if (*expression_path_ptr == path) {
          Impl::setChannelExpression(channel, value, *this);
          value_was_changed = true;
        }
      }
    });
  }

  if (value_was_changed) {
    updateTreeValues(tree_widget, tree_paths, scene_state);
    updateTreeDistanceErrorMarkerOptions(tree_widget, tree_paths, scene_state);
  }
  else {
    cerr << "handleTreeStringValueChanged: no match\n";
  }
}


static bool
setSceneStateBoolValue(
  SceneState &scene_state,
  const TreePath &path,
  bool value,
  const TreePaths &tree_paths,
  TreeWidget &tree_widget
)
{
  bool *solve_state_ptr = nullptr;
  Expression *expression_ptr = nullptr;
  const TreePath *expression_path_ptr = nullptr;

  auto solvable_element_function = [&](const auto &element){
    solve_state_ptr = solveStatePtr(element, scene_state);
    const auto &channel = elementChannel(element);

    expression_ptr =
      &channelExpression(channel, scene_state);

    expression_path_ptr = channelExpressionPathPtr(channel, tree_paths);

    assert(solve_state_ptr);
  };

  forSolvableSceneElement2(path, tree_paths, solvable_element_function);

  if (!solve_state_ptr) {
    return false;
  }

  *solve_state_ptr = value;
  assert(expression_ptr);
  *expression_ptr = "";
  tree_widget.setItemStringValue(*expression_path_ptr, "");
  return true;
}


void ObservedScene::handleTreeBoolValueChanged(const TreePath &path, bool value)
{
  bool value_was_changed =
    setSceneStateBoolValue(scene_state, path, value, tree_paths, tree_widget);

  if (value_was_changed) {
    solveScene();
    handleSceneStateChanged();
  }
}


static void
updateSolveFlag(
  TreeWidget &tree_widget,
  const TreePaths::Channel &channel_paths,
  bool solve_flag
)
{
  setTreeBoolValue(tree_widget, *channel_paths.maybe_solve_path, solve_flag);
}


static void
updateXYZSolveFlags(
  TreeWidget &tree_widget,
  const TreePaths::XYZChannels &xyz_paths,
  const SceneState::XYZSolveFlags &xyz_solve_flags
)
{
  updateSolveFlag(tree_widget, xyz_paths.x, xyz_solve_flags.x);
  updateSolveFlag(tree_widget, xyz_paths.y, xyz_solve_flags.y);
  updateSolveFlag(tree_widget, xyz_paths.z, xyz_solve_flags.z);
}


void ObservedScene::setSolveFlags(const TreeItemDescription &item, bool state)
{
  using ItemType = TreeItemDescription::Type;
  BodyIndex body_index = *item.maybe_body_index;

  SceneState::TransformSolveFlags &transform_solve_flags =
    scene_state.body(body_index).solve_flags;

  const TreePaths::Body &body_paths = tree_paths.body(body_index);

  if (item.type == ItemType::translation) {
    setAll(transform_solve_flags.translation, state);

    updateXYZSolveFlags(
      tree_widget,
      body_paths.translation,
      transform_solve_flags.translation
    );
  }

  if (item.type == ItemType::rotation) {
    setAll(transform_solve_flags.rotation, state);

    updateXYZSolveFlags(
      tree_widget,
      body_paths.rotation,
      transform_solve_flags.rotation
    );
  }
}


auto
ObservedScene::describePath(const TreePath &path) const
-> TreeItemDescription
{
  return Impl::describePath(path, tree_paths, scene_state);
}


void ObservedScene::updateSceneStateFromSceneObjects()
{
  ::updateSceneStateFromSceneObjects(scene_state, scene, scene_handles);
}
