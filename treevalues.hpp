#include "treepaths.hpp"
#include "treewidget.hpp"
#include "scenestate.hpp"
#include "stringvalue.hpp"
#include "channel.hpp"
#include "sceneelementdescription.hpp"


struct SceneTreeRef {
  TreeWidget &tree_widget;
  TreePaths &tree_paths;
};


extern TreePaths fillTree(TreeWidget &, const SceneState &);
extern void clearTree(TreeWidget &, const TreePaths &);

extern void
  updateTreeValues(
    TreeWidget &tree_widget,
    const TreePaths &tree_paths,
    const SceneState &state
  );

extern void
  setTreeBoolValue(
    TreeWidget &tree_widget,
    const TreePath &path,
    bool new_state
  );

extern Optional<Marker>
  markerFromEnumerationValue(int enumeration_value);

extern void
  createDistanceErrorInTree(DistanceErrorIndex,
    SceneTreeRef,
    const SceneState &
  );

extern void
  createMarkerInTree(
    MarkerIndex marker_index,
    SceneTreeRef,
    const SceneState &scene_state
  );

extern void
  createVariableInTree(
    VariableIndex,
    SceneTreeRef,
    const SceneState &scene_state
  );

extern void
  createMarkerItemInTree(
    MarkerIndex marker_index,
    SceneTreeRef,
    const SceneState &scene_state
  );

extern void createBodyInTree(BodyIndex, SceneTreeRef, const SceneState &);

extern void
  createBoxInTree(SceneTreeRef, const SceneState &, BodyIndex, BoxIndex);

extern void
  createLineInTree(
    SceneTreeRef,
    const SceneState &scene_state,
    BodyIndex body_index,
    LineIndex line_index
  );

extern void
  createMeshInTree(
    SceneTreeRef,
    const SceneState &,
    BodyIndex parent_body_index,
    MeshIndex
  );

extern void
  removeDistanceErrorFromTree(int distance_error_index, SceneTreeRef);

extern void removeMarkerFromTree(MarkerIndex, SceneTreeRef);
extern void removeVariableFromTree(VariableIndex, SceneTreeRef);

extern void removeMarkerItemFromTree(MarkerIndex marker_index, SceneTreeRef);

extern void
  removeBodyFromTree(SceneTreeRef, const SceneState &, BodyIndex body_index);

extern void
  removeBoxFromTree(SceneTreeRef, const SceneState &, BodyIndex, BoxIndex);

extern void
  removeLineFromTree(SceneTreeRef, const SceneState &, BodyIndex, LineIndex);

extern void
  removeMeshFromTree(SceneTreeRef, const SceneState &, BodyIndex, MeshIndex);

extern void
  createBodyBranchItemsInTree(
    BodyIndex body_index,
    SceneTreeRef,
    const SceneState &scene_state
  );

extern void
  removeBodyBranchItemsFromTree(
    BodyIndex body_index,
    SceneTreeRef,
    const SceneState &scene_state
  );

extern void
  updateTreeDistanceErrorMarkerOptions(
    TreeWidget &tree_widget,
    const TreePaths &tree_paths,
    const SceneState &scene_state
  );
  // This updates the list of markers that are shown as possible values for
  // the two markers of a distance error in the tree.

extern bool
  setSceneStateNumericValue(
    SceneState &scene_state,
    const TreePath &path,
    NumericValue value,
    const TreePaths &tree_paths
  );

extern bool
  setSceneStateStringValue(
    SceneState &scene_state,
    const TreePath &path,
    const StringValue &value,
    const TreePaths &tree_paths
  );

struct SolvableSceneElementVisitor {
  virtual void visit(const BodyTranslationComponent &) const = 0;
  virtual void visit(const BodyRotationComponent &) const = 0;
  virtual void visit(const BodyScale &) const = 0;
};


extern void
  forSolvableSceneElement(
    const TreePath &path,
    const TreePaths &tree_paths,
    const SolvableSceneElementVisitor &value_visitor
  );

template <typename Function>
struct SolvableSceneElementVisitorWrapper : SolvableSceneElementVisitor {
  const Function &f;

  SolvableSceneElementVisitorWrapper(const Function &f)
  : f(f)
  {
  }

  void visit(const BodyTranslationComponent &element) const override
  {
    f(element);
  }

  void visit(const BodyRotationComponent &element) const override
  {
    f(element);
  }

  void visit(const BodyScale &element) const override
  {
    f(element);
  }
};


template <typename Visitor>
void
forSolvableSceneElement2(
  const TreePath &path,
  const TreePaths &tree_paths,
  const Visitor &visitor
)
{
  forSolvableSceneElement(
    path,
    tree_paths,
    SolvableSceneElementVisitorWrapper<Visitor>(visitor)
  );
}


extern const TreePath &
  channelPath(
    const Channel &channel,
    const TreePaths &tree_paths
  );

const TreePath *channelExpressionPathPtr(const Channel &, const TreePaths &);


extern SceneElementDescription
  describeTreePath(
    const TreePath &path,
    const TreePaths &tree_paths
  );

extern void
  updateTreeBodyMeshPosition(
    TreeWidget &tree_widget,
    const TreePaths &tree_paths,
    const SceneState &scene_state,
    BodyMeshPosition body_mesh_position
  );
