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


#if 1
static float dot(Eigen::Vector3f a,Eigen::Vector3f b)
{
  return a.dot(b);
}
#endif


#if 1
static void
  updateDistanceError(
    TreeWidget &tree_widget,
    const TreePaths::DistanceError &distance_error_paths,
    const SceneState &state,
    int line_index
  )
{
  const SceneState::Line &state_line = state.lines[line_index];
  Point start = state.markerPredicted(state_line.start_marker_index);
  Point end = state.markerPredicted(state_line.end_marker_index);
  auto v = end-start;

  float distance = sqrt(dot(v,v));
  std::ostringstream label_stream;
  label_stream << "distance: " << distance;
  tree_widget.setItemLabel(distance_error_paths.distance, label_stream.str());
}
#endif


static void
  updateMarkers(
    TreeWidget &tree_widget,
    const TreePaths::Markers &marker_paths,
    const SceneState::Markers &markers
  )
{
  assert(marker_paths.size() == markers.size());
  int n_markers = marker_paths.size();

  for (int i=0; i!=n_markers; ++i) {
    updateXYZValues(
      tree_widget,
      marker_paths[i].position,
      vec3(markers[i].position)
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

  updateMarkers(tree_widget, tree_paths.markers, state.markers);

  int n_distance_errors = tree_paths.distance_errors.size();

  for (int i=0; i!=n_distance_errors; ++i) {
    updateDistanceError(tree_widget, tree_paths.distance_errors[i], state, i);
  }
}
