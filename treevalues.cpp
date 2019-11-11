#include "treevalues.hpp"

#include "eigenconv.hpp"
#include "rotationvector.hpp"
#include "numericvalue.hpp"
#include "sceneerror.hpp"
#include "indicesof.hpp"
#include "streamvector.hpp"
#include "removeindexfrom.hpp"

using std::cerr;
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


static TreePaths::Marker
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

  TreePaths::Position position_path =
    TreePaths::Position(createXYZ(tree_widget, childPath(path,1)));

  TreePaths::Marker marker_paths = {path, position_path};
  return marker_paths;
}


static vector<string> markerNames(const SceneState::Markers &state_markers)
{
  vector<string> result;

  for (const SceneState::Marker &marker : state_markers) {
    result.push_back(marker.name);
  }

  return result;
}


template <typename T>
static void appendTo(vector<T> &v, const vector<T> &new_values)
{
  v.insert(v.end(), new_values.begin(), new_values.end());
}


static TreeWidget::EnumerationOptions
  markerEnumerationOptions(const SceneState::Markers &state_markers)
{
  TreeWidget::EnumerationOptions result = {"None"};
  appendTo(result, markerNames(state_markers));
  return result;
}


Optional<MarkerIndex> markerIndexFromEnumerationValue(int enumeration_value)
{
  if (enumeration_value == 0) {
    return {};
  }
  else {
    return enumeration_value - 1;
  }
}


int enumerationValueFromMarkerIndex(Optional<MarkerIndex> maybe_marker_index)
{
  if (!maybe_marker_index) {
    return 0;
  }
  else {
    return *maybe_marker_index + 1;
  }
}


TreePaths::DistanceError
  createDistanceErrorInTree1(
    TreeWidget &tree_widget,
    TreePaths &/*tree_paths*/,
    const TreePath &path,
    const SceneState::Markers &state_markers,
    const SceneState::DistanceError &state_distance_error
  )
{
  Optional<MarkerIndex> optional_start_index =
    state_distance_error.optional_start_marker_index;

  Optional<MarkerIndex> optional_end_index =
    state_distance_error.optional_end_marker_index;

  tree_widget.createVoidItem(path,LabelProperties{"[DistanceError]"});
  TreePath start_path = childPath(path,0);
  TreePath end_path = childPath(path,1);
  TreePath distance_path = childPath(path,2);
  TreePath desired_distance_path = childPath(path,3);
  TreePath weight_path = childPath(path,4);
  TreePath error_path = childPath(path,5);
  LabelProperties start_label = {"start:"};
  LabelProperties end_label = {"end:"};
  LabelProperties distance_label = {"distance:"};
  LabelProperties desired_distance_label = {"desired_distance:"};
  LabelProperties weight_label = {"weight:"};
  LabelProperties error_label = {"error:"};
  TreeWidget::EnumerationOptions marker_options =
    markerEnumerationOptions(state_markers);

  tree_widget.createEnumerationItem(
    start_path,
    start_label,
    marker_options,
    enumerationValueFromMarkerIndex(optional_start_index)
  );

  tree_widget.createEnumerationItem(
    end_path,
    end_label,
    marker_options,
    enumerationValueFromMarkerIndex(optional_end_index)
  );

  tree_widget.createVoidItem(distance_path, distance_label);

  tree_widget.createNumericItem(
    desired_distance_path,
    desired_distance_label,
    /*value*/0,
    /*minimum_value*/0,
    /*maximum_value*/noMaximumNumericValue()
  );

  tree_widget.createNumericItem(
    weight_path,
    weight_label,
    /*value*/1,
    /*minimum_value*/0,
    /*maximum_value*/noMaximumNumericValue()
  );

  tree_widget.createVoidItem(error_path, error_label);

  return
    TreePaths::DistanceError{
      path,
      start_path,
      end_path,
      distance_path,
      desired_distance_path,
      weight_path,
      error_path
    };
}


void
  createDistanceErrorInTree(
    const SceneState::DistanceError &state_distance_error,
    TreeWidget &tree_widget,
    TreePaths &tree_paths,
    const SceneState &scene_state
  )
{
  TreePath &next_distance_error_path = tree_paths.next_distance_error_path;
  TreePath distance_error_path = next_distance_error_path;
  ++next_distance_error_path.back();

  tree_paths.distance_errors.push_back(
    createDistanceErrorInTree1(
      tree_widget,
      tree_paths,
      distance_error_path,
      scene_state.markers(),
      state_distance_error
    )
  );

  ++tree_paths.total_error[1];
}


static void
  offsetDistanceErrorPath(TreePaths::DistanceError &distance_error, int offset)
{
  distance_error.forEachMember(
    [&](TreePath (TreePaths::DistanceError::*member_ptr)){
      TreePath &member = distance_error.*member_ptr;
      member[1] += offset;
    }
  );
}


void
  removeDistanceErrorFromTree(
    int distance_error_index,
    TreePaths &tree_paths,
    TreeWidget &tree_widget
  )
{
  TreePaths::DistanceErrors &distance_errors = tree_paths.distance_errors;
  tree_widget.removeItem(distance_errors[distance_error_index].path);
  removeIndexFrom(distance_errors, distance_error_index);

  while (distance_error_index < int(distance_errors.size())) {
    TreePaths::DistanceError &distance_error =
      distance_errors[distance_error_index];

    offsetDistanceErrorPath(distance_error, -1);
    ++distance_error_index;
  }

  --tree_paths.next_distance_error_path[1];
  --tree_paths.total_error[1];
}


static void
  updateDistanceError(
    TreeWidget &tree_widget,
    const TreePaths::DistanceError &distance_error_paths,
    const SceneState::DistanceError &distance_error_state
  )
{
  {
    std::ostringstream label_stream;

    if (distance_error_state.maybe_distance) {
      label_stream << "distance: " << *distance_error_state.maybe_distance;
    }
    else {
      label_stream << "distance: N/A";
    }

    tree_widget.setItemLabel(distance_error_paths.distance, label_stream.str());
  }
  {
    std::ostringstream label_stream;
    label_stream << "error: " << distance_error_state.error;
    tree_widget.setItemLabel(distance_error_paths.error, label_stream.str());
  }
}


static string totalErrorLabel(float total_error)
{
  std::ostringstream label_stream;
  label_stream << "total_error: " << total_error;
  return label_stream.str();
}


void
  createMarkerInTree(
    TreeWidget &tree_widget,
    TreePaths &tree_paths,
    const SceneState &scene_state,
    MarkerIndex marker_index
  )
{
  const SceneState::Marker &state_marker = scene_state.marker(marker_index);
  assert(marker_index == MarkerIndex(tree_paths.markers.size()));

  if (state_marker.is_local) {
    TreePath &next_box_marker_path = tree_paths.next_box_marker_path;

    tree_paths.markers.push_back(
      createMarker(tree_widget,next_box_marker_path,state_marker.name)
    );

    ++next_box_marker_path.back();
  }
  else {
    TreePath &next_scene_marker_path = tree_paths.next_scene_marker_path;

    tree_paths.markers.push_back(
      createMarker(tree_widget,next_scene_marker_path,state_marker.name)
    );

    ++next_scene_marker_path.back();

    for (auto &distance_error : tree_paths.distance_errors) {
      offsetDistanceErrorPath(distance_error, 1);
    }

    ++tree_paths.next_distance_error_path[1];
    ++tree_paths.total_error[1];
  }
}


TreePaths fillTree(TreeWidget &tree_widget, const SceneState &scene_state)
{
  const NumericValue no_maximum = noMaximumNumericValue();

  TreePaths tree_paths;
  TreePath scene_path = {0};
  tree_widget.createVoidItem(scene_path,LabelProperties{"[Scene]"});
  tree_paths.path = scene_path;

  TreePath next_scene_child_path = childPath(scene_path, 0);
  TreePath box_transform_path = next_scene_child_path;
  ++next_scene_child_path.back();

  tree_widget.createVoidItem(
    box_transform_path,LabelProperties{"[Transform]"}
  );

  TreePath box_path = {0,0};
  TreePath box_translation_path = childPath(box_path,0);
  TreePath box_rotation_path = childPath(box_path,1);
  TreePath box_geometry_path = childPath(box_path, 2);
  tree_paths.box.path = box_path;

  TreePath box_scale_x_path = childPath(box_geometry_path, 0);
  TreePath box_scale_y_path = childPath(box_geometry_path, 1);
  TreePath box_scale_z_path = childPath(box_geometry_path, 2);

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
    box_geometry_path, LabelProperties{"[Box]"}
  );

  tree_widget.createNumericItem(
    box_scale_x_path,
    LabelProperties{"scale_x:"},
    /*value*/scene_state.box.scale_x,
    /*minimum_value*/0,
    no_maximum
  );

  tree_widget.createNumericItem(
    box_scale_y_path,
    LabelProperties{"scale_y:"},
    /*value*/scene_state.box.scale_y,
    /*minimum_value*/0,
    no_maximum
  );

  tree_widget.createNumericItem(
    box_scale_z_path,
    LabelProperties{"scale_z:"},
    /*value*/scene_state.box.scale_z,
    /*minimum_value*/0,
    no_maximum
  );

  tree_paths.box.geometry.path = box_geometry_path;
  tree_paths.box.geometry.scale.x = box_scale_x_path;
  tree_paths.box.geometry.scale.y = box_scale_y_path;
  tree_paths.box.geometry.scale.z = box_scale_z_path;

  tree_paths.next_distance_error_path = next_scene_child_path;
  tree_paths.next_scene_marker_path = next_scene_child_path;

  TreePath total_error_path = next_scene_child_path;
  ++next_scene_child_path.back();
  tree_paths.total_error = total_error_path;

  tree_widget.createVoidItem(
    total_error_path, LabelProperties{totalErrorLabel(0)}
  );

  {
    TreePath &next_box_marker_path = tree_paths.next_box_marker_path;
    next_box_marker_path = childPath(box_path, 3);

    for (auto marker_index : indicesOf(scene_state.markers())) {
      createMarkerInTree(tree_widget, tree_paths, scene_state, marker_index);
    }

    next_scene_child_path = tree_paths.next_scene_marker_path;
  }

  for (auto &state_distance_error : scene_state.distance_errors) {
    createDistanceErrorInTree(
      state_distance_error,
      tree_widget,
      tree_paths,
      scene_state
    );
  }

  assert(
    tree_paths.distance_errors.size() == scene_state.distance_errors.size()
  );

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
  const Transform &box_global = state.box.global;

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

  updateMarkers(tree_widget, tree_paths.markers, state.markers());

  for (auto i : indicesOf(tree_paths.distance_errors)) {
    updateDistanceError(
      tree_widget,
      tree_paths.distance_errors[i],
      state.distance_errors[i]
    );
  }

  tree_widget.setItemLabel(
    tree_paths.total_error, totalErrorLabel(state.total_error)
  );
}


void
  updateTreeDistanceErrorMarkerOptions(
    TreeWidget &tree_widget,
    const TreePaths &tree_paths,
    const SceneState &scene_state
  )
{
  const SceneState::Markers &state_markers = scene_state.markers();

  TreeWidget::EnumerationOptions marker_options =
    markerEnumerationOptions(state_markers);

  for (auto i : indicesOf(tree_paths.distance_errors)) {
    const TreePaths::DistanceError &distance_error_paths =
      tree_paths.distance_errors[i];

    const SceneState::DistanceError &distance_error_state =
      scene_state.distance_errors[i];

    const TreePath &start_path = distance_error_paths.start;
    const TreePath &end_path = distance_error_paths.end;

    Optional<MarkerIndex> optional_start_marker_index =
      distance_error_state.optional_start_marker_index;

    Optional<MarkerIndex> optional_end_marker_index =
      distance_error_state.optional_end_marker_index;

    int start_value =
      enumerationValueFromMarkerIndex(optional_start_marker_index);

    int end_value =
      enumerationValueFromMarkerIndex(optional_end_marker_index);

    tree_widget.setItemEnumerationValue(
      start_path, start_value, marker_options
    );

    tree_widget.setItemEnumerationValue(
      end_path, end_value, marker_options
    );
  }
}
