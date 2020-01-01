#include "treepaths.hpp"
#include "treewidget.hpp"
#include "scenestate.hpp"


extern TreePaths fillTree(TreeWidget &, const SceneState &);
extern void clearTree(TreeWidget &, const TreePaths &);


extern void
  updateTreeValues(
    TreeWidget &tree_widget,
    const TreePaths &tree_paths,
    const SceneState &state
  );

extern Optional<MarkerIndex>
  markerIndexFromEnumerationValue(int enumeration_value);

extern void
  createDistanceErrorInTree(
    const SceneState::DistanceError &state_distance_error,
    TreeWidget &tree_widget,
    TreePaths &tree_paths,
    const SceneState &scene_state
  );

extern void
  createMarkerInTree(
    TreeWidget &tree_widget,
    TreePaths &tree_paths,
    const SceneState &scene_state,
    MarkerIndex marker_index
  );

extern void
  createMarkerItemInTree(
    MarkerIndex marker_index,
    TreeWidget &tree_widget,
    TreePaths &tree_paths,
    const SceneState &scene_state
  );

extern void
  createBodyInTree(TreeWidget &, TreePaths &, const SceneState &, BodyIndex);

extern void
  createBodyItemsInTree(
    BodyIndex, TreeWidget &, TreePaths &, const SceneState &
  );

extern void
  removeDistanceErrorFromTree(
    int distance_error_index,
    TreePaths &tree_paths,
    TreeWidget &tree_widget
  );

extern void
  removeMarkerFromTree(
    MarkerIndex,
    TreePaths &tree_paths,
    TreeWidget &tree_widget
  );

extern void
  removeMarkerItemFromTree(
    MarkerIndex marker_index,
    TreeWidget &tree_widget,
    TreePaths &tree_paths
  );

extern void
  removeBodyFromTree(
    TreeWidget &tree_widget,
    TreePaths &tree_paths,
    const SceneState &,
    BodyIndex body_index
  );

extern void
  createBodyBranchItemsInTree(
    BodyIndex body_index,
    TreeWidget &tree_widget,
    TreePaths &tree_paths,
    const SceneState &scene_state
  );

extern void
  removeBodyBranchItemsFromTree(
    BodyIndex body_index,
    TreeWidget &tree_widget,
    TreePaths &tree_paths,
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
  setSceneStateValue(
    SceneState &scene_state,
    const TreePath &path,
    NumericValue value,
    const TreePaths &tree_paths
  );
