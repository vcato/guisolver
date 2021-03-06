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
#include "emplaceinto.hpp"
#include "vec3state.hpp"
#include "pointlink.hpp"
#include "solveflags.hpp"

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
  auto &body_state = scene_state.body(bodyOf(element).index);
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
  auto &body_state = scene_state.body(bodyOf(element).index);
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
  auto &body_state = scene_state.body(element.body.index);
  return &body_state.solve_flags.scale;
}


template <typename SceneState>
static auto *
solveStatePtr(
  const BodyMeshScaleComponent &element,
  SceneState &scene_state
)
{
  auto &body_state = scene_state.body(bodyOf(element).index);
  MeshIndex mesh_index = element.parent.body_mesh.index;
  auto &mesh_state = body_state.meshes[mesh_index];

  return
    xyzSolveStateComponentPtr(
      mesh_state.scale_solve_flags,
      element.component
    );
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
      auto &body_paths = tree_paths.body(bodyOf(element).index);

      tree_path_ptr =
        &*body_paths.translation.component(element.component).maybe_solve_path;
    }

    void visit(const BodyRotationComponent &element) const override
    {
      auto &body_paths = tree_paths.body(bodyOf(element).index);

      tree_path_ptr =
        &*body_paths.rotation.component(element.component).maybe_solve_path;
    }

    void visit(const BodyScale &element) const override
    {
      auto &body_paths = tree_paths.body(element.body.index);
      tree_path_ptr = &*body_paths.scale.maybe_solve_path;
    }

    void visit(const BodyMeshScaleComponent &element) const override
    {
      BodyMesh body_mesh = element.parent.body_mesh;
      auto &body_paths = tree_paths.body(body_mesh.body.index);

      tree_path_ptr =
        &*body_paths
        .meshes[body_mesh.index]
        .scale
        .component(element.component)
        .maybe_solve_path;
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
struct GeometryFor {
  const SceneHandles::Body &body_handles;
  const TreePaths::Body &body_paths;
  const F &f;

  void visitBox(BoxIndex box_index)
  {
    this->f(
      SceneObject{
        this->body_handles.transformHandle(),
        this->body_handles.boxes[box_index].handle,
      },
      this->body_paths.boxes[box_index].path
    );
  }

  void visitLine(LineIndex line_index)
  {
    this->f(
      SceneObject{
        this->body_handles.transformHandle(),
        this->body_handles.lines[line_index].handle,
      },
      this->body_paths.lines[line_index].path
    );
  }

  void visitMesh(MeshIndex mesh_index)
  {
    this->f(
      SceneObject{
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
      SceneObject{
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
      SceneObject{
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
      SceneObject{
        distance_error_handles.transform_handle,
        distance_error_handles.line_handle,
      },
      tree_paths.distance_errors[i].path
    );
  }

  if (scene_handles.maybe_points) {
    const vector<Scene::GeometryHandle> &point_sphere_handles =
      *scene_handles.maybe_points;

    const BodyMesh &body_mesh =
      scene_handles
      .maybe_manipulated_element
      .maybe_body_mesh_positions
      ->body_mesh;

    BodyIndex body_index = body_mesh.body.index;
    MeshIndex mesh_index = body_mesh.index;

    for (auto i : indicesOf(point_sphere_handles)) {
      const TreePaths::Positions &positions_paths =
        tree_paths.body(body_index).meshes[mesh_index].positions;

      f(
        SceneObject{
          {},
          point_sphere_handles[i]
        },
        positions_paths.elements[i].path
      );
    }
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

  if (describeTreePath(item_path, tree_paths).type == SceneElementDescription::Type::mesh_position) {
    return SceneObject();
  }

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

  static void
    setChannelExpression(
      const Channel &,
      const Expression &,
      ObservedScene &
    );

  static void
    handleMeshCreatedInState(
      BodyIndex body_index, MeshIndex mesh_index, ObservedScene &
    );

  static void attachProperDraggerToSelectedObject(ObservedScene &);
  static SceneObject selectedSceneObject(const Scene &);

  static Optional<ManipulationType>
    properManipulationForSceneObject(const SceneElementDescription &);

  static void updateTreeBodyMeshPosition(BodyMeshPosition, ObservedScene &);
  static void updateTreeMarkerItem(Marker, ObservedScene &);
  static void clearMark(ObservedScene &);
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

  bool is_manipulated =
    scene_handles.maybe_manipulated_element.maybe_marker_index == marker_index;

  if (is_manipulated) {
    removeExistingManipulator(scene_handles, scene);
  }

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

  if (scene_handles.maybe_manipulated_element.maybe_body_index == body_index) {
    removeExistingManipulator(scene_handles, scene);
  }

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
  updateTreeDistanceErrors(tree_widget, tree_paths, scene_state);
  updateSceneObjects(scene, scene_handles, scene_state);
  handleTreeSelectionChanged();
}


void ObservedScene::removeMarker(MarkerIndex marker_index)
{
  clearClipboard(*this);
  ObservedScene::removingMarker(*this, marker_index);
  scene_state.removeMarker(marker_index);
  updateSceneObjects(scene, scene_handles, scene_state);
  updateTreeDistanceErrors(tree_widget, tree_paths, scene_state);
  handleTreeSelectionChanged();
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
  handleTreeSelectionChanged();
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
  handleTreeSelectionChanged();
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
  handleTreeSelectionChanged();
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


static SceneState::MeshShape
createMeshShapeFromBox(const SceneState::Box &/*box_state*/)
{
  using MeshShape = SceneState::MeshShape;

  MeshShape::Positions positions = {
    {-0.5,   -0.5,   -0.5},
    {-0.5,   -0.5,    0.5},
    {-0.5,    0.5,   -0.5},
    {-0.5,    0.5,    0.5},
    { 0.5,   -0.5,   -0.5},
    { 0.5,   -0.5,    0.5},
    { 0.5,    0.5,   -0.5},
    { 0.5,    0.5,    0.5},
  };

  int faces[6][4] = {
    {8 ,  4 ,  2 ,  6},
    {8 ,  6 ,  5 ,  7},
    {8 ,  7 ,  3 ,  4},
    {4 ,  3 ,  1 ,  2},
    {1 ,  3 ,  7 ,  5},
    {2 ,  1 ,  5 ,  6},
  };

  MeshShape::Triangles triangles;

  for (int i=0; i!=6; ++i) {
    int v1 = faces[i][0] - 1;
    int v2 = faces[i][1] - 1;
    int v3 = faces[i][2] - 1;
    int v4 = faces[i][3] - 1;
    emplaceInto(triangles, i*2 + 0, v1, v2, v4);
    emplaceInto(triangles, i*2 + 1, v4, v2, v3);
  }

  return MeshShape{positions, triangles};
}


MeshIndex
ObservedScene::convertBoxToMesh(BodyIndex body_index, BoxIndex box_index)
{
  SceneState::Box box_state = scene_state.body(body_index).boxes[box_index];
  SceneState::MeshShape mesh_shape = createMeshShapeFromBox(box_state);
  removeBox(body_index, box_index);

  MeshIndex mesh_index = addMeshTo(body_index, mesh_shape);
  scene_state.body(body_index).meshes[mesh_index].scale = box_state.scale;
  updateSceneObjects(scene, scene_handles, scene_state);
  updateTreeValues(tree_widget, tree_paths, scene_state);
  return mesh_index;
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
ObservedScene::Impl::updateTreeBodyMeshPosition(
  BodyMeshPosition arg,
  ObservedScene &observed_scene
)
{
  TreeWidget &tree_widget = observed_scene.tree_widget;
  TreePaths &tree_paths = observed_scene.tree_paths;
  SceneState &scene_state = observed_scene.scene_state;
  ::updateTreeBodyMeshPosition(tree_widget, tree_paths, scene_state, arg);
}


void
ObservedScene::Impl::updateTreeMarkerItem(
  Marker marker,
  ObservedScene &observed_scene
)
{
  ::updateTreeMarkerItem(
    marker,
    observed_scene.tree_widget,
    observed_scene.tree_paths,
    observed_scene.scene_state
  );
}


void ObservedScene::Impl::clearMark(ObservedScene &observed_scene)
{
  SceneState &scene_state = observed_scene.scene_state;

  if (scene_state.maybe_marked_body_mesh_position) {
    BodyMeshPosition old_mark = *scene_state.maybe_marked_body_mesh_position;
    scene_state.maybe_marked_body_mesh_position.reset();
    Impl::updateTreeBodyMeshPosition(old_mark, observed_scene);
  }

  if (scene_state.maybe_marked_marker) {
    Marker old_mark = *scene_state.maybe_marked_marker;
    scene_state.maybe_marked_marker.reset();
    updateTreeMarkerItem(old_mark, observed_scene);
  }
}


void ObservedScene::markMeshPosition(BodyMeshPosition arg)
{
  Impl::clearMark(*this);
  scene_state.maybe_marked_body_mesh_position = arg;
  Impl::updateTreeBodyMeshPosition(arg, *this);
}


void ObservedScene::markMarker(Marker arg)
{
  Impl::clearMark(*this);
  scene_state.maybe_marked_marker = arg;
  updateTreeMarkerItem(arg, tree_widget, tree_paths, scene_state);
}


Optional<ManipulationType>
ObservedScene::Impl::properManipulationForSceneObject(
  const SceneElementDescription &item
)
{
  if (item.maybe_mesh_position_index) {
    return ManipulationType::translate;
  }

  if (item.type == SceneElementDescription::Type::mesh_positions) {
    return ManipulationType::points;
  }

  if (item.hasRotationAncestor()) {
    return ManipulationType::rotate;
  }

  if (item.hasTranslationAncestor()) {
    return ManipulationType::translate;
  }

  if (item.hasScaleAncestor()) {
    return {};
  }

  if (item.maybe_box_index) {
    return ManipulationType::translate_and_scale;
  }

  if (item.maybe_mesh_index) {
    if (item.type == SceneElementDescription::Type::mesh_positions) {
      return {};
    }
    else {
      return ManipulationType::translate_and_scale;
    }
  }

  if (item.maybe_body_index) {
    return ManipulationType::translate;
  }

  if (item.maybe_marker_index) {
    return ManipulationType::translate;
  }

  return {};
}


Optional<ManipulationType>
ObservedScene::properManipulationForSelectedObject() const
{
  const ObservedScene &observed_scene = *this;
  Scene &scene = observed_scene.scene;
  SceneObject scene_object = Impl::selectedSceneObject(scene);

  SceneElementDescription item =
    observed_scene.describePath(*tree_widget.selectedItem());

  return Impl::properManipulationForSceneObject(item);
}


SceneObject ObservedScene::Impl::selectedSceneObject(const Scene &scene)
{
  Optional<GeometryHandle> selected_geometry = scene.selectedGeometry();
  Optional<TransformHandle> selected_transform;

  if (!selected_geometry) {
    selected_transform = scene.selectedTransform();
  }

  return {selected_transform, selected_geometry};
}


#if 0
static Scene::TransformHandle
transformFor(const SceneObject &scene_object, const Scene &scene)
{
  if (scene_object.maybe_transform_handle) {
    return *scene_object.maybe_transform_handle;
  }

  assert(scene_object.maybe_geometry_handle);
  return scene.parentTransform(*scene_object.maybe_geometry_handle);
}
#endif


#if 0
static Optional<BodyIndex>
maybeBodyIndexForTransform(
  TransformHandle handle,
  const SceneHandles &scene_handles
)
{
  for (auto body_index : indicesOf(scene_handles.bodies)) {
    const SceneHandles::Body &body_handles = *scene_handles.bodies[body_index];

    if (body_handles.transform_handle == handle) {
      return body_index;
    }
  }

  return {};
}
#endif


static void
addBodyMeshPositionManipulator(
  BodyIndex body_index,
  MeshIndex mesh_index,
  MeshPositionIndex position_index,
  const SceneState &scene_state,
  Scene &scene,
  SceneHandles &scene_handles
)
{
  TransformHandle parent_transform =
    scene.parentTransform(
      scene_handles.body(body_index).meshes[mesh_index].handle
    );

  TransformHandle manipulator =
    scene.createTranslateManipulator(parent_transform);

  updateBodyMeshPositionManipulator(
    manipulator,
    Body(body_index).mesh(mesh_index).position(position_index),
    scene_state,
    scene
  );

  assert(!scene_handles.maybe_translate_manipulator);
  scene_handles.maybe_translate_manipulator = manipulator;

  scene_handles.maybe_manipulated_element.maybe_body_mesh_position =
    Body(body_index).mesh(mesh_index).position(position_index);
}


static void
addBodyTranslateManipulator(
  Scene &scene,
  SceneHandles &scene_handles,
  const BodyIndex body_index
)
{
  TransformHandle body_transform =
    scene_handles.body(body_index).transform_handle;

  TransformHandle parent_transform =
    scene.parentTransform(body_transform);

  if (scene_handles.maybe_manipulated_element.maybe_body_index) {
    cerr << "Body already has a manipulator\n";
    return;
  }

  assert(!scene_handles.maybe_translate_manipulator);

  TransformHandle manipulator_handle =
    scene.createTranslateManipulator(parent_transform);

  assert(!scene_handles.maybe_manipulated_element.maybe_body_index);
  scene_handles.maybe_translate_manipulator = manipulator_handle;
  scene_handles.maybe_manipulated_element.maybe_body_index = body_index;

  updateBodyTranslateManipulator(
    scene, body_transform, manipulator_handle
  );
}


static void
addMarkerTranslateManipulator(
  MarkerIndex marker_index,
  SceneHandles &scene_handles,
  Scene &scene
)
{
  if (scene_handles.maybe_manipulated_element.maybe_marker_index) {
    assert(false);
  }

  TransformHandle marker_transform_handle =
    scene_handles.marker(marker_index).transformHandle();

  TransformHandle parent_transform =
    scene.parentTransform(marker_transform_handle);

  TransformHandle manipulator_handle =
    scene.createTranslateManipulator(parent_transform);

  scene_handles.maybe_translate_manipulator = manipulator_handle;
  scene_handles.maybe_manipulated_element.maybe_marker_index = marker_index;

  updateMarkerTranslateManipulator(
    scene, marker_transform_handle, manipulator_handle
  );
}


static void
addBodyRotateManipulator(
  Scene &scene,
  SceneHandles &scene_handles,
  const BodyIndex body_index
)
{
  TransformHandle body_transform =
    scene_handles.body(body_index).transform_handle;

  TransformHandle parent_transform =
    scene.parentTransform(body_transform);

  if (scene_handles.maybe_manipulated_element.maybe_body_index) {
    cerr << "Body already has a manipulator\n";
    return;
  }

  assert(!scene_handles.maybe_rotate_manipulator);

  TransformHandle manipulator_handle =
    scene.createRotateManipulator(parent_transform);

  assert(!scene_handles.maybe_manipulated_element.maybe_body_index);
  scene_handles.maybe_rotate_manipulator = manipulator_handle;
  scene_handles.maybe_manipulated_element.maybe_body_index = body_index;

  updateBodyRotateManipulator(
    scene, body_transform, manipulator_handle
  );
}


static void
addBodyBoxScaleManipulator(
  SceneHandles &scene_handles,
  BodyIndex body_index,
  BoxIndex box_index,
  Scene &scene
)
{
  const SceneHandles::Box &box_handles =
    scene_handles.body(body_index).boxes[box_index];

  GeometryHandle box_geometry_handle = box_handles.handle;

  TransformHandle parent_transform =
    scene.parentTransform(box_geometry_handle);

  GeometryHandle manipulator =
    scene.createScaleManipulator(parent_transform);

  scene_handles.maybe_scale_manipulator = manipulator;

  scene_handles.maybe_manipulated_element.maybe_body_box =
    BodyBox(body_index, box_index);

  updateBodyBoxScaleManipulator(
    scene, box_geometry_handle, manipulator
  );
}


static void
updateMeshPointsGeometry(
  BodyMeshPositions body_mesh_positions,
  const SceneState &scene_state,
  const SceneHandles &scene_handles,
  Scene &scene
)
{
  const vector<GeometryHandle> &sphere_handles = *scene_handles.maybe_points;
  BodyMesh body_mesh = body_mesh_positions.body_mesh;
  BodyIndex body_index = body_mesh.body.index;
  MeshIndex mesh_index = body_mesh.index;

  const SceneState::MeshShape::Positions &positions =
    scene_state.body(body_index).meshes[mesh_index].shape.positions;

  float global_scale = bodyGlobalScale(body_index, scene_state);
  int n_positions = positions.size();

  for (int i=0; i!=n_positions; ++i) {
    Vec3 adjusted_position =
      bodyMeshPositionRelativeToBody(
        Body(body_index).mesh(mesh_index).position(i),
        scene_state
      );

    adjusted_position.x *= global_scale;
    adjusted_position.y *= global_scale;
    adjusted_position.z *= global_scale;

    GeometryHandle handle = sphere_handles[i];
    scene.setGeometryCenter(handle, adjusted_position);
    scene.setGeometryScale(handle, Vec3(0.1, 0.1, 0.1));
  }
}


static void
addBodyMeshPointsGeometry(
  SceneHandles &scene_handles,
  BodyIndex body_index,
  MeshIndex mesh_index,
  Scene &scene,
  const SceneState &scene_state
)
{
  const SceneState::MeshShape::Positions &positions =
    scene_state.body(body_index).meshes[mesh_index].shape.positions;

  int n_positions = positions.size();
  vector<GeometryHandle> sphere_handles;

  for (int i=0; i!=n_positions; ++i) {
    Scene::GeometryHandle mesh_geometry_handle =
      scene_handles.body(body_index).meshes[mesh_index].handle;

    TransformHandle parent_transform =
      scene.parentTransform(mesh_geometry_handle);

    GeometryHandle handle = scene.createSphere(parent_transform);

    sphere_handles.push_back(handle);
  }

  assert(!scene_handles.maybe_points);
  scene_handles.maybe_points = sphere_handles;

  scene_handles.maybe_manipulated_element.maybe_body_mesh_positions =
    Body(body_index).mesh(mesh_index).positions();

  updateMeshPointsGeometry(
    Body(body_index).mesh(mesh_index).positions(),
    scene_state,
    scene_handles,
    scene
  );
}


static void
addBodyMeshScaleManipulator(
  SceneHandles &scene_handles,
  BodyIndex body_index,
  MeshIndex mesh_index,
  Scene &scene
)
{
  const SceneHandles::Mesh &mesh_handles =
    scene_handles.body(body_index).meshes[mesh_index];

  TransformHandle parent_transform =
    scene.parentTransform(mesh_handles.handle);

  GeometryHandle manipulator =
    scene.createScaleManipulator(parent_transform);

  scene_handles.maybe_scale_manipulator = manipulator;

  scene_handles.maybe_manipulated_element.maybe_body_mesh =
    Body{body_index}.mesh(mesh_index);

  updateBodyMeshScaleManipulator(
    scene, mesh_handles.handle, manipulator
  );
}


static void
addManipulator(
  ManipulationType manipulation_type,
  const SceneElementDescription &item,
  Scene &scene,
  SceneHandles &scene_handles,
  const SceneState &scene_state
)
{
  switch (manipulation_type) {
    case ManipulationType::translate:
      if (item.maybe_body_index) {
        BodyIndex body_index = *item.maybe_body_index;

        if (item.maybe_mesh_position_index) {
          MeshIndex mesh_index = *item.maybe_mesh_index;
          MeshPositionIndex position_index = *item.maybe_mesh_position_index;

          addBodyMeshPointsGeometry(
            scene_handles, body_index, mesh_index, scene, scene_state
          );

          addBodyMeshPositionManipulator(
            body_index, mesh_index, position_index, scene_state, scene,
            scene_handles
          );
        }
        else {
          addBodyTranslateManipulator(scene, scene_handles, body_index);
        }
      }
      else if (item.maybe_marker_index) {
        MarkerIndex marker_index = *item.maybe_marker_index;
        addMarkerTranslateManipulator(marker_index, scene_handles, scene);
      }
      else {
        cerr << "addManipulator:\n";
        cerr << "  Translating unknown object\n";
      }

      break;
    case ManipulationType::rotate:
      if (item.maybe_body_index) {
        BodyIndex body_index = *item.maybe_body_index;
        addBodyRotateManipulator(scene, scene_handles, body_index);
      }
      else {
        cerr << "Rotating unknown object\n";
      }

      break;
    case ManipulationType::translate_and_scale:
      if (item.maybe_box_index) {
        BoxIndex box_index = *item.maybe_box_index;
        BodyIndex body_index = *item.maybe_body_index;
        addBodyBoxScaleManipulator(scene_handles, body_index, box_index, scene);
      }
      else if (item.maybe_mesh_index) {
        MeshIndex mesh_index = *item.maybe_mesh_index;
        BodyIndex body_index = *item.maybe_body_index;

        addBodyMeshScaleManipulator(
          scene_handles, body_index, mesh_index, scene
        );
      }
      else {
        assert(false); // not implemented
      }

      break;
    case ManipulationType::points:
      if (item.maybe_mesh_index) {
        MeshIndex mesh_index = *item.maybe_mesh_index;
        BodyIndex body_index = *item.maybe_body_index;

        addBodyMeshPointsGeometry(
          scene_handles, body_index, mesh_index, scene, scene_state
        );
      }
      else {
        assert(false); // not implemented
      }

      break;
  }
}


void
ObservedScene::Impl::attachProperDraggerToSelectedObject(
  ObservedScene &observed_scene
)
{
  Scene &scene = observed_scene.scene;
  const TreeWidget &tree_widget = observed_scene.tree_widget;
  SceneHandles &scene_handles = observed_scene.scene_handles;

  SceneElementDescription item =
    observed_scene.describePath(*tree_widget.selectedItem());

  Optional<ManipulationType> maybe_manipulation_type =
    Impl::properManipulationForSceneObject(item);

  removeExistingManipulator(scene_handles, scene);

  if (!maybe_manipulation_type) {
    return;
  }

  addManipulator(
    *maybe_manipulation_type,
    item,
    scene,
    scene_handles,
    observed_scene.scene_state
  );
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

  // Could the scene object actually be a mesh position?

  SceneObject scene_object =
    sceneObjectForTreeItem(
      *maybe_selected_item_path,
      tree_paths,
      scene_handles,
      scene_state
    );

  if (scene_object.maybe_geometry_handle) {
    scene.selectGeometry(*scene_object.maybe_geometry_handle);
  }
  else if (scene_object.maybe_transform_handle) {
    scene.selectTransform(*scene_object.maybe_transform_handle);
  }
  else {
    cerr << "handleTreeSelectionChanged: No scene object found\n";
    scene.selectTransform(scene.top());
  }

  Impl::attachProperDraggerToSelectedObject(observed_scene);
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
    removeExistingManipulator(scene_handles, scene);
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

  Impl::attachProperDraggerToSelectedObject(observed_scene);
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
          .body(bodyOf(channel).index)
          .transform
          .translation
          .component(channel.component);
    }

    void visit(const BodyRotationChannel &channel) const override
    {
      value_ptr = &
        scene_state
          .body(bodyOf(channel).index)
          .transform
          .rotation
          .component(channel.component);
    }

    void visit(const BodyScaleChannel &channel) const override
    {
      value_ptr = &
        scene_state
          .body(channel.body.index)
          .transform
          .scale;
    }

    void visit(const BodyBoxScaleChannel &channel) const override
    {
      value_ptr = &
        scene_state
          .body(bodyOf(channel).index)
          .boxes[bodyBoxOf(channel).index]
          .scale
          .component(channel.component);
    }

    void visit(const BodyBoxCenterChannel &channel) const override
    {
      value_ptr = &
        scene_state
          .body(bodyOf(channel).index)
          .boxes[bodyBoxOf(channel).index]
          .center
          .component(channel.component);
    }

    void visit(const MarkerPositionChannel &channel) const override
    {
      value_ptr = &
        scene_state
          .marker(markerOf(channel).index)
          .position
          .component(channel.component);
    }

    void visit(const BodyMeshScaleChannel &channel) const override
    {
      value_ptr = &
        scene_state
        .body(bodyOf(channel).index)
        .meshes[bodyMeshOf(channel).index]
        .scale
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
    Body body{body_index};
    BodyTranslation body_translation{body};
    BodyRotation body_rotation{body};
    f(BodyTranslationChannel{{body_translation, XYZComponent::x}});
    f(BodyTranslationChannel{{body_translation, XYZComponent::y}});
    f(BodyTranslationChannel{{body_translation, XYZComponent::z}});
    f(BodyRotationChannel{{body_rotation, XYZComponent::x}});
    f(BodyRotationChannel{{body_rotation, XYZComponent::y}});
    f(BodyRotationChannel{{body_rotation, XYZComponent::z}});
    f(BodyScaleChannel(BodyScale{body_index}));
    BoxIndex n_boxes = scene_state.bodies()[body_index].boxes.size();

    for (BoxIndex box_index = 0; box_index != n_boxes; ++box_index) {
      BodyBox body_box{body_index, box_index};
      f(BodyBoxScaleChannel{{{body_box}, XYZComponent::x}});
      f(BodyBoxScaleChannel{{{body_box}, XYZComponent::y}});
      f(BodyBoxScaleChannel{{{body_box}, XYZComponent::z}});
      f(BodyBoxCenterChannel{{{body_box}, XYZComponent::x}});
      f(BodyBoxCenterChannel{{{body_box}, XYZComponent::y}});
      f(BodyBoxCenterChannel{{{body_box}, XYZComponent::z}});
    }
  }

  for (MarkerIndex marker_index : indicesOf(scene_state.markers())) {
    MarkerPosition marker_position{marker_index};
    f(MarkerPositionChannel{{marker_position, XYZComponent::x}});
    f(MarkerPositionChannel{{marker_position, XYZComponent::y}});
    f(MarkerPositionChannel{{marker_position, XYZComponent::z}});
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
      state_distance_error.setStart(markerFromEnumerationValue(value));
    }

    if (path == distance_error_paths.end) {
      state_distance_error.setEnd(markerFromEnumerationValue(value));
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


void
ObservedScene::Impl::handleMeshCreatedInState(
  BodyIndex body_index, MeshIndex mesh_index, ObservedScene &observed_scene
)
{
  ::createMeshInScene(
    observed_scene.scene,
    observed_scene.scene_handles, body_index, mesh_index,
    observed_scene.scene_state
  );

  ::createMeshInTree(
    sceneTree(observed_scene),
    observed_scene.scene_state, body_index, mesh_index
  );
}


MeshIndex
ObservedScene::addMeshTo(
  BodyIndex body_index, const SceneState::MeshShape &mesh_shape
)
{
  MeshIndex mesh_index =
    scene_state.body(body_index).createMesh(mesh_shape);

  Impl::handleMeshCreatedInState(body_index, mesh_index, *this);
  return mesh_index;
}


MeshIndex ObservedScene::addMeshTo(BodyIndex body_index, const Mesh &mesh)
{
  return addMeshTo(body_index, meshShapeStateFromMesh(mesh));
}


Marker ObservedScene::createMarker(Optional<Body> maybe_body)
{
  Optional<BodyIndex> maybe_body_index = maybeBodyIndex(maybe_body);
  ObservedScene &observed_scene = *this;
  TreePaths &tree_paths = observed_scene.tree_paths;
  SceneState &scene_state = observed_scene.scene_state;
  TreeWidget &tree_widget = observed_scene.tree_widget;
  MarkerIndex marker_index = scene_state.createMarker(maybe_body_index);
  createMarkerInScene(marker_index, observed_scene);
  createMarkerInTree(marker_index, observed_scene);
  updateTreeDistanceErrors(tree_widget, tree_paths, scene_state);
  return Marker(marker_index);
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
  removeExistingManipulator(scene_handles, scene);
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


static TreePath treePath(Body body, const TreePaths &tree_paths)
{
  return tree_paths.body(body.index).path;
}


static TreePath treePath(Marker marker, const TreePaths &tree_paths)
{
  return tree_paths.marker(marker.index).path;
}


static TreePath
treePath(DistanceError distance_error, const TreePaths &tree_paths)
{
  return tree_paths.distance_errors[distance_error.index].path;
}


static TreePath
treePath(Variable variable, const TreePaths &tree_paths)
{
  return tree_paths.variables[variable.index].path;
}


static TreePath
treePath(BodyBox body_box, const TreePaths &tree_paths)
{
  return tree_paths.body(body_box.body.index).boxes[body_box.index].path;
}


static TreePath
treePath(BodyLine body_line, const TreePaths &tree_paths)
{
  return tree_paths.body(body_line.body.index).lines[body_line.index].path;
}


static TreePath
treePath(BodyMesh body_mesh, const TreePaths &tree_paths)
{
  return tree_paths.body(body_mesh.body.index).meshes[body_mesh.index].path;
}


template <typename Element>
static void select(Element element, ObservedScene &observed_scene)
{
  TreePath path = treePath(element, observed_scene.tree_paths);
  observed_scene.tree_widget.selectItem(path);
  observed_scene.handleTreeSelectionChanged();
}


void ObservedScene::select(Marker marker)
{
  ::select(marker, *this);
}


void ObservedScene::selectBody(BodyIndex body_index)
{
  ::select(Body{body_index}, *this);
}


void ObservedScene::selectMarker(MarkerIndex marker_index)
{
  ::select(Marker{marker_index}, *this);
}


void ObservedScene::selectDistanceError(DistanceErrorIndex index)
{
  ::select(DistanceError{index}, *this);
}


void ObservedScene::selectVariable(VariableIndex variable_index)
{
  ::select(Variable{variable_index}, *this);
}


void ObservedScene::selectBox(BodyIndex body_index, BoxIndex box_index)
{
  ::select(BodyBox{body_index, box_index}, *this);
}


void ObservedScene::selectLine(BodyIndex body_index, LineIndex line_index)
{
  ::select(BodyLine{body_index, line_index}, *this);
}


void ObservedScene::selectMesh(BodyIndex body_index, MeshIndex mesh_index)
{
  ::select(Body(body_index).mesh(mesh_index), *this);
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
  updateTreeDistanceErrors(tree_widget, tree_paths, scene_state);
  return new_marker_index;
}


BodyIndex
ObservedScene::duplicateBodyWithDistanceErrors(BodyIndex body_index)
{
  MarkerNameMap marker_name_map;

  BodyIndex new_body_index =
    ObservedScene::duplicateBody(body_index, marker_name_map, *this);

  for (auto &map_entry : marker_name_map) {
    MarkerIndex marker1_index =
      *findMarkerWithName(scene_state, map_entry.first);

    MarkerIndex marker2_index =
      *findMarkerWithName(scene_state, map_entry.second);

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


DistanceError
ObservedScene::createDistanceError(
  Optional<PointLink> optional_start,
  Optional<PointLink> optional_end,
  Optional<Body> optional_body
)
{
  ObservedScene &observed_scene = *this;

  DistanceErrorIndex index =
    scene_state.createDistanceError(maybeBodyIndex(optional_body));

  SceneState::DistanceError &distance_error =
    scene_state.distance_errors[index];

  distance_error.setStart(optional_start);
  distance_error.setEnd(optional_end);
  observed_scene.update_errors_function(scene_state);
  createDistanceErrorInScene(scene, scene_handles, scene_state, index);

  createDistanceErrorInTree(
    index,
    { tree_widget, tree_paths },
    scene_state
  );

  updateSceneObjects(scene, scene_handles, scene_state);
  updateTreeValues(tree_widget, tree_paths, scene_state);
  return DistanceError(index);
}


DistanceErrorIndex
ObservedScene::addDistanceError(
  Optional<MarkerIndex> optional_start_marker_index,
  Optional<MarkerIndex> optional_end_marker_index,
  Optional<BodyIndex> optional_body_index
)
{
  Optional<Marker> optional_start_marker =
    maybeMarker(optional_start_marker_index);

  Optional<Marker> optional_end_marker = maybeMarker(optional_end_marker_index);
  Optional<PointLink> optional_start = maybePointLink(optional_start_marker);
  Optional<PointLink> optional_end = maybePointLink(optional_end_marker);
  Optional<Body> optional_body = maybeBody(optional_body_index);

  return createDistanceError(optional_start, optional_end, optional_body).index;
}


void
ObservedScene::setDistanceErrorPointToMark(
  DistanceError distance_error,
  DistanceErrorPoint point
)
{
  SceneState::DistanceError &distance_error_state =
    scene_state.distance_errors[distance_error.index];

  if (scene_state.maybe_marked_body_mesh_position) {
    BodyMeshPosition marked_body_mesh_position =
      *scene_state.maybe_marked_body_mesh_position;

    switch (point) {
      case DistanceErrorPoint::start:
        distance_error_state.setStart(marked_body_mesh_position);
        break;
      case DistanceErrorPoint::end:
        distance_error_state.setEnd(marked_body_mesh_position);
        break;
    }
  }
  else if (scene_state.maybe_marked_marker) {
    Marker marked_marker = *scene_state.maybe_marked_marker;

    switch (point) {
      case DistanceErrorPoint::start:
        distance_error_state.setStart(marked_marker);
        break;
      case DistanceErrorPoint::end:
        distance_error_state.setEnd(marked_marker);
        break;
    }
  }
  else {
    return;
  }

  updateTreeDistanceErrors(tree_widget, tree_paths, scene_state);
  solveScene();
  handleSceneStateChanged();
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


void
ObservedScene::handleBodyMeshPositionStateChanged(
  BodyMeshPosition body_mesh_position
)
{
  updateTreeBodyMeshPosition(
    tree_widget, tree_paths, scene_state, body_mesh_position
  );

  if (scene_handles.maybe_points) {
    Optional<BodyMeshPositions> &maybe_body_mesh_positions =
      scene_handles.maybe_manipulated_element.maybe_body_mesh_positions;

    if (maybe_body_mesh_positions) {
      updateMeshPointsGeometry(
        *maybe_body_mesh_positions,
        scene_state,
        scene_handles,
        scene
      );
    }
  }
}


VariableIndex ObservedScene::addVariable()
{
  VariableIndex index = scene_state.createVariable();
  createVariableInTree(index, sceneTree(*this), scene_state);
  return index;
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

    SceneElementDescription description = describePath(path);

    if (description.maybe_mesh_position_index) {
      // To save time, handleSceneStateChanged() doesn't update every position
      // in all the meshes, so if we've changed a mesh position we need to
      // explicitly update that in the scene explicitly.

      BodyIndex body_index = *description.maybe_body_index;
      MeshIndex mesh_index = *description.maybe_mesh_index;
      MeshPositionIndex position_index = *description.maybe_mesh_position_index;

      updateBodyMeshPositionInScene(
        body_index,
        mesh_index,
        position_index,
        scene,
        scene_handles,
        scene_state
      );

      BodyMeshPosition body_mesh_position =
        Body(body_index).mesh(mesh_index).position(position_index);

      if (
        scene_handles.maybe_manipulated_element.maybe_body_mesh_position
        == body_mesh_position
      ) {
        updateBodyMeshPositionManipulator(
          *scene_handles.maybe_translate_manipulator,
          body_mesh_position,
          scene_state,
          scene
        );
      }
    }

    observed_scene.handleSceneStateChanged();
  }
  else {
    cerr << "handleTreeNumericValueChanged: unknown path\n";
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
    updateTreeDistanceErrors(tree_widget, tree_paths, scene_state);
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


void
ObservedScene::setSolveFlags(const SceneElementDescription &item, bool state)
{
  using ItemType = SceneElementDescription::Type;
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
-> SceneElementDescription
{
  return ::describeTreePath(path, tree_paths);
}


void ObservedScene::updateSceneStateFromSceneObjects()
{
  ::updateSceneStateFromSceneObjects(scene_state, scene, scene_handles);
}


#if 0
void
ObservedScene::setDistanceErrorStartToMark(
  DistanceError distance_error
)
{
  if (scene_state.maybe_marked_body_mesh_position) {
    scene_state[distance_error].optional_start =
      *scene_state.maybe_marked_body_mesh_position;
  }
}
#endif


static Optional<TransformHandle> selectedObjectTransform(const Scene &scene)
{
  Optional<GeometryHandle>
    maybe_selected_geometry = scene.selectedGeometry();

  if (!maybe_selected_geometry) {
    return scene.selectedTransform();
  }

  return scene.parentTransform(*maybe_selected_geometry);
}


template <typename SceneState, typename F>
static void
  forEachSolveFlagAffectingBody(
    BodyIndex body_index,
    SceneState &state,
    const F &f,
    const typename SceneState::TransformSolveFlags &visit
  )
{
  forEachSolveFlagInTransform(state.body(body_index).solve_flags, f, visit);

  Optional<BodyIndex> maybe_parent_index =
    state.body(body_index).maybe_parent_index;

  if (!maybe_parent_index) {
    return;
  }

  return forEachSolveFlagAffectingBody(*maybe_parent_index, state, f, visit);
}


template <typename SceneState, typename F>
static void
forEachSolveFlagAffectingHandle(
  TransformHandle handle,
  const SceneHandles &scene_handles,
  SceneState &state,
  Optional<ManipulationType> maybe_manipulation_type,
  const F &f
)
{
  // If the handle is for a body that is a child of the box, then it
  // is going to be affected by the solve.
  for (auto i : indicesOf(state.bodies())) {
    if (handle == scene_handles.body(i).transformHandle()) {
      typename SceneState::TransformSolveFlags visit;
      setAll(visit, true);

      if (maybe_manipulation_type == ManipulationType::translate) {
        // If we're using a translation manipulator, the rotations don't
        // affect it.
        setAll(visit.rotation, false);
      }

      forEachSolveFlagAffectingBody(i, state, f, visit);
    }
  }

  for (auto i : indicesOf(state.markers())) {
    if (handle == scene_handles.marker(i).transformHandle()) {
      typename SceneState::TransformSolveFlags visit;
      setAll(visit, true);
      Optional<BodyIndex> maybe_body_index = state.marker(i).maybe_body_index;

      if (maybe_body_index) {
        forEachSolveFlagAffectingBody(*maybe_body_index, state, f, visit);
      }
    }
  }
}


static void handleSceneChanging(ObservedScene &observed_scene)
{
  bool update_scene_state = true;

  SceneHandles &scene_handles = observed_scene.scene_handles;
  Scene &scene = observed_scene.scene;
  SceneState &state = observed_scene.scene_state;

  if (scene_handles.maybe_translate_manipulator) {
    TransformHandle translate_manipulator =
      *scene_handles.maybe_translate_manipulator;

    if (scene_handles.maybe_manipulated_element.maybe_body_index) {
      BodyIndex body_index =
        *scene_handles.maybe_manipulated_element.maybe_body_index;

      TransformHandle body_transform_handle =
        scene_handles.body(body_index).transform_handle;

      Scene::Point manipulator_position =
        scene.translation(translate_manipulator);

      scene.setTranslation(body_transform_handle, manipulator_position);
    }
    else if (scene_handles.maybe_manipulated_element.maybe_marker_index) {
      MarkerIndex marker_index =
        *scene_handles.maybe_manipulated_element.maybe_marker_index;

      TransformHandle marker_transform_handle =
        scene_handles.marker(marker_index).transformHandle();

      Scene::Point manipulator_position =
        scene.translation(translate_manipulator);

      scene.setTranslation(marker_transform_handle, manipulator_position);
    }
    else if (scene_handles.maybe_manipulated_element.maybe_body_mesh_position) {
      BodyMeshPosition body_mesh_position =
        *scene_handles.maybe_manipulated_element.maybe_body_mesh_position;

      updateBodyMeshPositionFromManipulator(
        translate_manipulator,
        body_mesh_position,
        state,
        scene,
        scene_handles
      );

      observed_scene.handleBodyMeshPositionStateChanged(body_mesh_position);
      return;
    }
    else {
      cerr << "handleSceneChanging: Unknown translate manipulator\n";
      return;
    }
  }
  else if (scene_handles.maybe_rotate_manipulator) {
    TransformHandle rotate_manipulator =
      *scene_handles.maybe_rotate_manipulator;

    if (scene_handles.maybe_manipulated_element.maybe_body_index) {
      BodyIndex body_index =
        *scene_handles.maybe_manipulated_element.maybe_body_index;

      TransformHandle body_transform_handle =
        scene_handles.body(body_index).transform_handle;

      CoordinateAxes manipulator_position =
        scene.coordinateAxes(rotate_manipulator);

      scene.setCoordinateAxes(body_transform_handle, manipulator_position);
    }
    else {
      cerr << "handleSceneChanging: Unknown rotate manipulator\n";
    }
  }
  else if (scene_handles.maybe_scale_manipulator) {
    GeometryHandle manipulator = *scene_handles.maybe_scale_manipulator;

    if (scene_handles.maybe_manipulated_element.maybe_body_box) {
      BodyBox body_box =
        *scene_handles.maybe_manipulated_element.maybe_body_box;

      GeometryHandle box_handle =
        scene_handles.body(body_box.body.index).boxes[body_box.index].handle;

      updateBodyBoxFromScaleManipulator(box_handle, manipulator, scene);
    }
    else if (scene_handles.maybe_manipulated_element.maybe_body_mesh) {
      BodyMesh body_mesh =
        *scene_handles.maybe_manipulated_element.maybe_body_mesh;

      BodyIndex body_index = body_mesh.body.index;

      auto &mesh_handles =
        scene_handles.body(body_index).meshes[body_mesh.index];

      Scene::MeshHandle mesh_handle = mesh_handles.handle;

      SceneState::Mesh &mesh_state =
        state.body(body_index).meshes[body_mesh.index];

      float body_global_scale = bodyGlobalScale(body_index, state);

      updateBodyMeshStateFromScaleManipulator(
        mesh_state,
        manipulator,
        mesh_handle,
        scene,
        body_global_scale
      );

      updateMeshInScene(scene, mesh_state, mesh_handles, body_global_scale);
      update_scene_state = false;
    }
    else {
      cerr << "handleSceneChanging: Unknown scale manipulator\n";
    }
  }
  else {
    cerr << "handleSceneChanging: Unknown manipulator\n";
    return;
  }

  Optional<TransformHandle> maybe_transform_handle =
    selectedObjectTransform(scene);

  assert(maybe_transform_handle);
    // How could the scene be changing if nothing was selected?

  TransformHandle transform_handle = *maybe_transform_handle;

  if (update_scene_state) {
    observed_scene.updateSceneStateFromSceneObjects();
  }

  vector<bool> old_flags;

  // Disable any transforms that would move the handle and remember the
  // old state.

  Optional<ManipulationType> maybe_manipulation_type =
    observed_scene.properManipulationForSelectedObject();

  forEachSolveFlagAffectingHandle(
    transform_handle,
    scene_handles,
    state,
    maybe_manipulation_type,
    [&](bool &arg){
      old_flags.push_back(arg);
      arg = false;
    }
  );

  observed_scene.solveScene();

  // Restore the old solve states.
  {
    vector<bool>::const_iterator iter = old_flags.begin();

    forEachSolveFlagAffectingHandle(
      transform_handle, scene_handles, state,
      maybe_manipulation_type,
      [&](bool &arg){
        arg = *iter++;
      }
    );

    assert(iter == old_flags.end());
  }

  observed_scene.handleSceneStateChanged();
}


void ObservedScene::handleSceneChanging()
{
  ::handleSceneChanging(*this);
}


void ObservedScene::handleSceneChanged()
{
  solveScene();
  handleSceneStateChanged();
}
