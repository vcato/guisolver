#include "treevalues.hpp"

#include "eigenconv.hpp"
#include "rotationvector.hpp"
#include "numericvalue.hpp"
#include "sceneerror.hpp"


using std::string;
using LabelProperties = TreeWidget::LabelProperties;


static TreePaths::XYZ
  createXYZ(TreeWidget &tree_widget, const TreePath &parent_path)
{
  const NumericValue no_minimum = noMinimumNumericValue();
  const NumericValue no_maximum = noMaximumNumericValue();

  TreePaths::XYZ xyz_paths;
  xyz_paths.path = parent_path;
  xyz_paths.x = childPath(parent_path,0);
  xyz_paths.y = childPath(parent_path,1);
  xyz_paths.z = childPath(parent_path,2);

  tree_widget.createNumericItem(
    xyz_paths.x,LabelProperties{"x:"},0,no_minimum, no_maximum
  );

  tree_widget.createNumericItem(
    xyz_paths.y,LabelProperties{"y:"},0,no_minimum, no_maximum
  );

  tree_widget.createNumericItem(
    xyz_paths.z,LabelProperties{"z:"},0,no_minimum, no_maximum
  );

  return xyz_paths;
}


static TreePaths::Position
  createMarker(
    TreeWidget &tree_widget,
    const TreePath &path,
    const string &name
  )
{
  tree_widget.createVoidItem(path,LabelProperties{"[Marker]"});

  tree_widget.createVoidItem(
    childPath(path,0),LabelProperties{"name: \"" + name + "\""}
  );

  tree_widget.createVoidItem(childPath(path,1),LabelProperties{"position: []"});

  return TreePaths::Position(createXYZ(tree_widget, childPath(path,1)));
}


static float dot(Eigen::Vector3f a,Eigen::Vector3f b)
{
  return a.dot(b);
}


static TreePaths::DistanceError
  createDistanceError(
    TreeWidget &tree_widget,
    const TreePath &path,
    const string &start_name,
    const string &end_name
  )
{
  tree_widget.createVoidItem(path,LabelProperties{"[DistanceError]"});
  TreePath start_path = childPath(path,0);
  TreePath end_path = childPath(path,1);
  TreePath distance_path = childPath(path,2);
  TreePath desired_distance_path = childPath(path,3);
  TreePath error_path = childPath(path,4);
  LabelProperties start_label = {"start: " + start_name};
  LabelProperties end_label = {"end: " + end_name};
  LabelProperties distance_label = {"distance: 0"};
  LabelProperties desired_distance_label = {"desired_distance: 0"};
  LabelProperties error_label = {"error: 0"};
  tree_widget.createVoidItem(start_path, start_label);
  tree_widget.createVoidItem(end_path, end_label);
  tree_widget.createVoidItem(distance_path, distance_label);
  tree_widget.createVoidItem(desired_distance_path, desired_distance_label);
  tree_widget.createVoidItem(error_path, error_label);
  return TreePaths::DistanceError{path, distance_path, error_path};
}


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
  {
    const TreePath &path = distance_error_paths.distance;
    std::ostringstream label_stream;
    label_stream << "distance: " << distance;
    tree_widget.setItemLabel(path, label_stream.str());
  }
  {
    float error = distanceError(start,end);
    const TreePath &path = distance_error_paths.error;
    std::ostringstream label_stream;
    label_stream << "error: " << error;
    tree_widget.setItemLabel(path, label_stream.str());
  }
}


TreePaths fillTree(TreeWidget &tree_widget)
{
  TreePaths tree_paths;

  tree_widget.createVoidItem({0},LabelProperties{"[Scene]"});

  tree_widget.createVoidItem(
    {0,0},LabelProperties{"[Transform]"}
  );

  TreePath box_path = {0,0};
  TreePath box_translation_path = childPath(box_path,0);
  TreePath box_rotation_path = childPath(box_path,1);

  tree_paths.box.path = box_path;

  tree_widget.createVoidItem(
    box_translation_path,LabelProperties{"translation: []"}
  );

  tree_paths.box.translation =
    TreePaths::Translation(createXYZ(tree_widget, box_translation_path));

  tree_widget.createVoidItem(
    box_rotation_path,LabelProperties{"rotation: []"}
  );

  tree_paths.box.rotation =
    TreePaths::Rotation(createXYZ(tree_widget, box_rotation_path));

  tree_widget.createVoidItem(
    {0,0,2},LabelProperties{"[Box]"}
  );

  tree_paths.markers[0].position = createMarker(tree_widget,{0,0,3},"local1");
  tree_paths.markers[1].position = createMarker(tree_widget,{0,0,4},"local2");
  tree_paths.markers[2].position = createMarker(tree_widget,{0,0,5},"local3");
  tree_paths.markers[3].position = createMarker(tree_widget,{0,1},"global1");
  tree_paths.markers[4].position = createMarker(tree_widget,{0,2},"global2");
  tree_paths.markers[5].position = createMarker(tree_widget,{0,3},"global3");

  tree_paths.distance_errors[0] =
    createDistanceError(tree_widget,{0,4},"local1","global1");

  tree_paths.distance_errors[1] =
    createDistanceError(tree_widget,{0,5},"local2","global2");

  tree_paths.distance_errors[2] =
    createDistanceError(tree_widget,{0,6},"local3","global3");

  return tree_paths;
}


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
