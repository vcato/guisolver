#include "treepaths.hpp"
#include "treewidget.hpp"
#include "scenestate.hpp"
#include "stringvalue.hpp"
#include "channel.hpp"


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

extern Optional<MarkerIndex>
  markerIndexFromEnumerationValue(int enumeration_value);

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

struct SolvableSceneValueVisitor {
  virtual void visitBodyTranslationComponent(BodyIndex, XYZComponent) const = 0;
  virtual void visitBodyRotationComponent(BodyIndex, XYZComponent) const = 0;
  virtual void visitBodyScale(BodyIndex) const = 0;
};


extern void
  forSolvableSceneValue(
    const TreePath &path,
    const TreePaths &tree_paths,
    const SolvableSceneValueVisitor &value_visitor
  );

extern const TreePath &
  channelPath(
    const Channel &channel,
    const TreePaths &tree_paths
  );

const TreePath *channelExpressionPathPtr(const Channel &, const TreePaths &);
