#include "updatetreevalues.hpp"

#include "eigenconv.hpp"
#include "rotationvector.hpp"


static void
  updateXYZValues(
    QtTreeWidget &tree_widget,
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
    QtTreeWidget &tree_widget,
    const TreePaths::Translation &translation_paths,
    const Point &translation
  )
{
  updateXYZValues(tree_widget, translation_paths, vec3(translation));
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
  updateXYZValues(tree_widget, rotation_paths, r_deg);
}


void
  updateTreeValues(
    QtTreeWidget &tree_widget,
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

  {
    assert(tree_paths.locals.size() == state.locals.size());
    int n_locals = tree_paths.locals.size();

    for (int i=0; i!=n_locals; ++i) {
      updateXYZValues(
        tree_widget,
        tree_paths.locals[i].position,
        vec3(state.locals[i])
      );
    }
  }

  {
    assert(tree_paths.globals.size() == state.globals.size());
    int n_globals = tree_paths.globals.size();

    for (int i=0; i!=n_globals; ++i) {
      updateXYZValues(
        tree_widget,
        tree_paths.globals[i].position,
        vec3(state.globals[i])
      );
    }
  }
}
