#include "updatetreevalues.hpp"


void
  updateTreeValues(
    const TreePaths &tree_paths,
    QtTreeWidget &tree_widget,
    const SceneState &state
  )
{
  tree_widget.setItemNumericValue(
    tree_paths.box.translation.x,
    state.box_global.translation().x()
  );

  tree_widget.setItemNumericValue(
    tree_paths.box.translation.y,
    state.box_global.translation().y()
  );

  tree_widget.setItemNumericValue(
    tree_paths.box.translation.z,
    state.box_global.translation().z()
  );
}
