#include "updatetreevalues.hpp"

#include "eigenconv.hpp"
#include "rotationvector.hpp"


static void
  updateTranslationValues(
    QtTreeWidget &tree_widget,
    const TreePaths::Translation &translation_paths,
    const Point &translation
  )
{
  tree_widget.setItemNumericValue(translation_paths.x, translation.x());
  tree_widget.setItemNumericValue(translation_paths.y, translation.y());
  tree_widget.setItemNumericValue(translation_paths.z, translation.z());
}


static void
  updateRotationValues(
    QtTreeWidget &tree_widget,
    const TreePaths::Rotation &rotation_paths,
    const Eigen::Matrix3f &rotation
  )
{
  Vec3 r_rad = rotationVector(rotation);
  Vec3 r_deg = r_rad * (180/M_PI);
  tree_widget.setItemNumericValue(rotation_paths.x, r_deg.x);
  tree_widget.setItemNumericValue(rotation_paths.y, r_deg.y);
  tree_widget.setItemNumericValue(rotation_paths.z, r_deg.z);
}


void
  updateTreeValues(
    QtTreeWidget &tree_widget,
    const TreePaths &tree_paths,
    const SceneState &state
  )
{
  {
    const TreePaths::Translation &translation_paths =
      tree_paths.box.translation;

    const Point &translation = state.box_global.translation();
    updateTranslationValues(tree_widget,translation_paths,translation);
  }

  {
    const Eigen::Matrix3f &rotation = state.box_global.rotation();
    const TreePaths::Rotation &rotation_paths = tree_paths.box.rotation;
    updateRotationValues(tree_widget,rotation_paths, rotation);
  }
}
