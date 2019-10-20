#include "updatetreevalues.hpp"

#include "eigenconv.hpp"
#include "rotationvector.hpp"


static void
  updateXYZValues(
    TreeWidget &tree_widget,
    const TreePaths::XYZ &paths,
    const Vec3 &value
  )
{
  tree_widget.setItemNumericValue(paths.x, value.x);
  tree_widget.setItemNumericValue(paths.y, value.y);
  tree_widget.setItemNumericValue(paths.z, value.z);
}


static void
  updateTranslationValues(
    TreeWidget &tree_widget,
    const TreePaths::Translation &translation_paths,
    const Point &translation
  )
{
  updateXYZValues(tree_widget, translation_paths, vec3(translation));
}


static void
  updateRotationValues(
    TreeWidget &tree_widget,
    const TreePaths::Rotation &rotation_paths,
    const Eigen::Matrix3f &rotation
  )
{
  Vec3 r_rad = rotationVector(rotation);
  Vec3 r_deg = r_rad * (180/M_PI);
  updateXYZValues(tree_widget, rotation_paths, r_deg);
}


static void
  updateMarkers(
    TreeWidget &tree_widget,
    const TreePaths::Markers &marker_paths,
    const SceneState::Points &marker_positions
  )
{
  assert(marker_paths.size() == marker_positions.size());
  int n_markers = marker_paths.size();

  for (int i=0; i!=n_markers; ++i) {
    updateXYZValues(
      tree_widget,
      marker_paths[i].position,
      vec3(marker_positions[i])
    );
  }
}


void
  updateTreeValues(
    TreeWidget &tree_widget,
    const TreePaths &tree_paths,
    const SceneState &state
  )
{
  const Transform &box_global = state.box_global;

  {
    const TreePaths::Translation &translation_paths =
      tree_paths.box.translation;

    const Point &translation = box_global.translation();
    updateTranslationValues(tree_widget, translation_paths, translation);
  }

  {
    const Eigen::Matrix3f &rotation = box_global.rotation();
    const TreePaths::Rotation &rotation_paths = tree_paths.box.rotation;
    updateRotationValues(tree_widget, rotation_paths, rotation);
  }

  updateMarkers(tree_widget, tree_paths.locals, state.locals);
  updateMarkers(tree_widget, tree_paths.globals, state.globals);
}
