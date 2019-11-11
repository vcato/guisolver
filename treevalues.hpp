#include "treepaths.hpp"
#include "treewidget.hpp"
#include "scenestate.hpp"


extern TreePaths fillTree(TreeWidget &, const SceneState &);


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

void
  createMarkerInTree(
    TreeWidget &tree_widget,
    TreePaths &tree_paths,
    const SceneState &scene_state,
    MarkerIndex marker_index
  );

extern void
  removeDistanceErrorFromTree(
    int distance_error_index,
    TreePaths &tree_paths,
    TreeWidget &tree_widget
  );

extern void
  updateTreeDistanceErrorMarkerOptions(
    TreeWidget &tree_widget,
    const TreePaths &tree_paths,
    const SceneState &scene_state
  );
