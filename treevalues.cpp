#include "treevalues.hpp"

#include "eigenconv.hpp"
#include "rotationvector.hpp"
#include "numericvalue.hpp"
#include "sceneerror.hpp"
#include "indicesof.hpp"
#include "streamvector.hpp"
#include "removeindexfrom.hpp"
#include "startswith.hpp"
#include "numericvaluelimits.hpp"
#include "maketransform.hpp"
#include "transformstate.hpp"

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


static void
  updatePathAfterRemoval(TreePath &path_to_update, const TreePath &path_removed)
{
  if (startsWith(path_to_update, parentPath(path_removed))) {
    if (path_to_update.size() >= path_removed.size()) {
      auto depth = path_removed.size() - 1;

      if (path_to_update[depth] > path_removed[depth]) {
        --path_to_update[depth];
      }
    }
  }
}


static void
  updatePathBeforeInsertion(
    TreePath &path_to_update,
    const TreePath &path_to_insert
  )
{
  if (startsWith(path_to_update, parentPath(path_to_insert))) {
    if (path_to_update.size() >= path_to_insert.size()) {
      auto depth = path_to_insert.size() - 1;

      if (path_to_update[depth] >= path_to_insert[depth]) {
        ++path_to_update[depth];
      }
    }
  }
}


template <typename Visitor>
static void visitPaths(TreePath &path, const Visitor &visitor)
{
  visitor(path);
}


template <typename T, typename Visitor>
static void visitPaths(vector<T> &v, const Visitor &visitor)
{
  for (auto i : indicesOf(v)) {
    visitPaths(v[i], visitor);
  }
}


template <typename T, typename Visitor>
static void visitPaths(const vector<T> &v, const Visitor &visitor)
{
  for (auto i : indicesOf(v)) {
    visitPaths(v[i], visitor);
  }
}


template <typename Object, typename Visitor>
static void visitPaths(Object &object, const Visitor &visitor)
{
  object.forEachMember(
    [&](auto member_ptr){
      visitPaths(object.*member_ptr, visitor);
    }
  );
}


static void
  handlePathRemoval(TreePaths &tree_paths, const TreePath &path_removed)
{
  visitPaths(
    tree_paths,
    [&](TreePath &path){ updatePathAfterRemoval(path, path_removed); }
  );
}


static void
  handlePathInsertion(TreePaths &tree_paths, const TreePath &path_to_insert)
{
  visitPaths(
    tree_paths,
    [&](TreePath &path){ updatePathBeforeInsertion(path, path_to_insert); }
  );
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
  handlePathInsertion(tree_paths, distance_error_path);
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
}


void
  removeDistanceErrorFromTree(
    int distance_error_index,
    TreePaths &tree_paths,
    TreeWidget &tree_widget
  )
{
  TreePaths::DistanceErrors &distance_errors = tree_paths.distance_errors;
  TreePath distance_error_path = distance_errors[distance_error_index].path;
  tree_widget.removeItem(distance_error_path);
  removeIndexFrom(distance_errors, distance_error_index);
  handlePathRemoval(tree_paths, distance_error_path);

  updatePathAfterRemoval(
    tree_paths.next_distance_error_path,
    distance_error_path
  );
}


extern void
  removeMarkerFromTree(
    MarkerIndex marker_index,
    TreePaths &tree_paths,
    TreeWidget &tree_widget
  )
{
  TreePaths::Markers &markers = tree_paths.markers;
  TreePath marker_path = markers[marker_index].path;
  tree_widget.removeItem(marker_path);
  removeIndexFrom(markers, marker_index);
  handlePathRemoval(tree_paths, marker_path);
  updatePathAfterRemoval(tree_paths.next_distance_error_path, marker_path);
  updatePathAfterRemoval(tree_paths.next_scene_marker_path, marker_path);

  for (BodyIndex body_index : indicesOf(tree_paths.bodies)) {
    updatePathAfterRemoval(
      tree_paths.bodies[body_index].next_marker_path,
      marker_path
    );
  }
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


static TreePath &
  nextMarkerInsertionPoint(
    TreePaths &tree_paths,
    Optional<BodyIndex> maybe_body_index
  )
{
  if (maybe_body_index) {
    return tree_paths.bodies[*maybe_body_index].next_marker_path;
  }
  else {
    return tree_paths.next_scene_marker_path;
  }
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
  Optional<BodyIndex> maybe_body_index = state_marker.maybe_body_index;

  TreePath &insert_point =
    nextMarkerInsertionPoint(tree_paths, maybe_body_index);

  TreePath marker_path = insert_point;
  ++insert_point.back();

  if (!maybe_body_index) {
    ++tree_paths.next_distance_error_path.back();
  }

  handlePathInsertion(tree_paths, marker_path);

  tree_paths.markers.push_back(
    createMarker(tree_widget, marker_path, state_marker.name)
  );
}


static TreePaths::Body
createBodyItem(
  const TreePath &body_path,
  const SceneState::Body &body_state,
  TreeWidget &tree_widget
)
{
  const NumericValue no_maximum = noMaximumNumericValue();
  TreePaths::Body body_paths;
  TreePath transform_path = body_path;

  tree_widget.createVoidItem(
    transform_path, LabelProperties{"[Transform]"}
  );

  TreePath translation_path = childPath(transform_path,0);
  TreePath rotation_path = childPath(transform_path,1);
  TreePath geometry_path = childPath(transform_path, 2);
  TreePath next_body_path = childPath(transform_path, 3);
  TreePath next_marker_path = childPath(transform_path, 3);
  body_paths.path = body_path;

  TreePath scale_x_path = childPath(geometry_path, 0);
  TreePath scale_y_path = childPath(geometry_path, 1);
  TreePath scale_z_path = childPath(geometry_path, 2);

  tree_widget.createVoidItem(
    translation_path,LabelProperties{"translation: []"}
  );

  body_paths.translation =
    TreePaths::Translation(createXYZ(tree_widget, translation_path));

  tree_widget.createVoidItem(
    rotation_path,LabelProperties{"rotation: []"}
  );

  body_paths.rotation =
    TreePaths::Rotation(createXYZ(tree_widget, rotation_path));

  tree_widget.createVoidItem(
    geometry_path, LabelProperties{"[Box]"}
  );

  tree_widget.createNumericItem(
    scale_x_path,
    LabelProperties{"scale_x:"},
    /*value*/body_state.scale.x,
    /*minimum_value*/0,
    no_maximum
  );

  tree_widget.createNumericItem(
    scale_y_path,
    LabelProperties{"scale_y:"},
    /*value*/body_state.scale.y,
    /*minimum_value*/0,
    no_maximum
  );

  tree_widget.createNumericItem(
    scale_z_path,
    LabelProperties{"scale_z:"},
    /*value*/body_state.scale.z,
    /*minimum_value*/0,
    no_maximum
  );

  body_paths.geometry.path = geometry_path;
  body_paths.geometry.scale.x = scale_x_path;
  body_paths.geometry.scale.y = scale_y_path;
  body_paths.geometry.scale.z = scale_z_path;

  body_paths.next_body_path = next_body_path;
  body_paths.next_marker_path = next_marker_path;

  return body_paths;
}


static void
  createSceneBodyInTree(
    const BodyIndex body_index,
    TreePaths &tree_paths,
    TreeWidget &tree_widget,
    const SceneState &scene_state
  )
{
  const TreePath body_path = tree_paths.next_scene_body_path;
  assert(BodyIndex(tree_paths.bodies.size()) == body_index);
  const SceneState::Body &body_state = scene_state.body(body_index);

  tree_paths.bodies.push_back(
    createBodyItem(body_path, body_state, tree_widget)
  );

  ++tree_paths.next_scene_body_path.back();
  ++tree_paths.next_scene_marker_path.back();
  ++tree_paths.next_distance_error_path.back();
  ++tree_paths.total_error.back();
}


static void
  createChildBodyInTree(
    const BodyIndex body_index,
    TreePaths &tree_paths,
    TreeWidget &tree_widget,
    const SceneState &scene_state
  )
{
  BodyIndex parent_body_index =
    *scene_state.body(body_index).maybe_parent_index;

  const TreePath body_path =
    tree_paths.bodies[parent_body_index].next_body_path;

  handlePathInsertion(tree_paths, body_path);

  assert(BodyIndex(tree_paths.bodies.size()) == body_index);
  const SceneState::Body &body_state = scene_state.body(body_index);

  tree_paths.bodies.push_back(
    createBodyItem(body_path, body_state, tree_widget)
  );

  ++tree_paths.bodies[parent_body_index].next_body_path.back();
  ++tree_paths.bodies[parent_body_index].next_marker_path.back();
}


void
createBodyInTree(
  TreeWidget &tree_widget,
  TreePaths &tree_paths,
  const SceneState &scene_state,
  BodyIndex body_index
)
{
  const SceneState::Body &state_body = scene_state.body(body_index);

  if (!state_body.maybe_parent_index) {
    createSceneBodyInTree(body_index, tree_paths, tree_widget, scene_state);
  }
  else {
    createChildBodyInTree(body_index, tree_paths, tree_widget, scene_state);
  }
}


TreePaths fillTree(TreeWidget &tree_widget, const SceneState &scene_state)
{
  TreePaths tree_paths;
  TreePath scene_path = {0};
  tree_widget.createVoidItem(scene_path,LabelProperties{"[Scene]"});
  tree_paths.path = scene_path;

  TreePath next_scene_child_path = childPath(scene_path, 0);
  tree_paths.next_distance_error_path = next_scene_child_path;
  tree_paths.next_scene_marker_path = next_scene_child_path;
  tree_paths.next_scene_body_path = next_scene_child_path;
  tree_paths.total_error = next_scene_child_path;

  tree_widget.createVoidItem(
    tree_paths.total_error, LabelProperties{totalErrorLabel(0)}
  );

  for (auto body_index : indicesOf(scene_state.bodies())) {
    createBodyInTree(tree_widget, tree_paths, scene_state, body_index);
  }

  for (auto marker_index : indicesOf(scene_state.markers())) {
    createMarkerInTree(tree_widget, tree_paths, scene_state, marker_index);
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


void clearTree(TreeWidget &tree_widget, const TreePaths &tree_paths)
{
  tree_widget.removeItem(tree_paths.path);
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
    const TranslationState &translation
  )
{
  updateXYZValues(tree_widget, translation_paths, vec3(translation));
}


static void
  updateRotationValues(
    TreeWidget &tree_widget,
    const TreePaths::Rotation &rotation_paths,
    const RotationState &rotation
  )
{
  updateXYZValues(tree_widget, rotation_paths, rotationValuesDeg(rotation));
}


static void
  updateMarker(
    MarkerIndex i,
    TreeWidget &tree_widget,
    const TreePaths::Markers &marker_paths,
    const SceneState::Markers &markers
  )
{
  updateXYZValues(
    tree_widget,
    marker_paths[i].position,
    vec3(markers[i].position)
  );
}


static void
updateBody(
  TreeWidget &tree_widget,
  const TreePaths &tree_paths,
  const SceneState &state,
  BodyIndex body_index
)
{
  const TransformState &global = state.body(body_index).transform;
  const TreePaths::Body &body_paths = tree_paths.bodies[body_index];
  {
    const TreePaths::Translation &translation_paths = body_paths.translation;
    const TranslationState &translation = translationStateOf(global);
    updateTranslationValues(tree_widget, translation_paths, translation);
  }
  {
    const TreePaths::Rotation &rotation_paths = body_paths.rotation;
    const RotationState &rotation = rotationStateOf(global);
    updateRotationValues(tree_widget, rotation_paths, rotation);
  }
}


void
  updateTreeValues(
    TreeWidget &tree_widget,
    const TreePaths &tree_paths,
    const SceneState &state
  )
{
  for (auto body_index : indicesOf(state.bodies())) {
    updateBody(tree_widget, tree_paths, state, body_index);
  }

  for (auto i : indicesOf(state.markers())) {
    updateMarker(i, tree_widget, tree_paths.markers, state.markers());
  }

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


static void
  setVectorValue(
    Eigen::Vector3f &v,
    const TreePath &path,
    NumericValue value,
    const TreePaths::XYZ &xyz_path
  )
{
  if (path == xyz_path.x) {
    v.x() = value;
  }
  else if (path == xyz_path.y) {
    v.y() = value;
  }
  else if (path == xyz_path.z) {
    v.z() = value;
  }
  else {
    assert(false);
  }
}


static void
  setVectorValue(
    SceneState::XYZ &v,
    const TreePath &path,
    NumericValue value,
    const TreePaths::XYZ &xyz_path
  )
{
  if (path == xyz_path.x) {
    v.x = value;
  }
  else if (path == xyz_path.y) {
    v.y = value;
  }
  else if (path == xyz_path.z) {
    v.z = value;
  }
  else {
    assert(false);
  }
}


static void
  setTranslationValue(
    TransformState &box_global,
    const TreePath &path,
    NumericValue value,
    const TreePaths::Translation &xyz_path
  )
{
  setVectorValue(box_global.translation, path, value, xyz_path);
}


static void
  setRotationValue(
    TransformState &box_global,
    const TreePath &path,
    NumericValue value,
    const TreePaths::Rotation &xyz_path
  )
{
  setVectorValue(box_global.rotation, path, value, xyz_path);
}


static bool
  setTransformValue(
    TransformState &box_global,
    const TreePath &path,
    NumericValue value,
    const TreePaths::Body &box_paths
  )
{
  if (startsWith(path,box_paths.translation.path)) {
    setTranslationValue(box_global, path, value, box_paths.translation);
    return true;
  }

  if (startsWith(path,box_paths.rotation.path)) {
    setRotationValue(box_global, path, value, box_paths.rotation);
    return true;
  }

  return false;
}


static bool
  setMarkerValue(
    const TreePath &path,
    NumericValue value,
    const TreePaths::Marker &marker_path,
    SceneState::Marker &marker_state
  )
{
  if (startsWith(path,marker_path.position.path)) {
    setVectorValue(marker_state.position, path, value, marker_path.position);
    return true;
  }

  return false;
}


static bool
  setMarkersValue(
    const TreePath &path,
    NumericValue value,
    const TreePaths::Markers &markers_paths,
    SceneState &scene_state
  )
{
  for (auto i : indicesOf(markers_paths)) {
    bool value_was_set =
      setMarkerValue(
        path,
        value,
        markers_paths[i],
        scene_state.marker(i)
      );

    if (value_was_set) {
      return true;
    }
  }

  return false;
}



static bool
  setDistanceErrorValue(
    SceneState::DistanceError &distance_error_state,
    const TreePath &path,
    NumericValue value,
    const TreePaths::DistanceError &distance_error_paths
  )
{
  if (startsWith(path, distance_error_paths.desired_distance)) {
    distance_error_state.desired_distance = value;
    return true;
  }

  if (startsWith(path, distance_error_paths.weight)) {
    distance_error_state.weight = value;
    return true;
  }

  return false;
}


static bool
  setDistanceErrorsValue(
    SceneState::DistanceErrors &distance_error_states,
    const TreePath &path,
    NumericValue value,
    const TreePaths::DistanceErrors &distance_errors_paths
  )
{
  assert(distance_errors_paths.size() == distance_error_states.size());

  for (auto i : indicesOf(distance_errors_paths)) {
    bool value_was_set =
      setDistanceErrorValue(
        distance_error_states[i],
        path,
        value,
        distance_errors_paths[i]
      );

    if (value_was_set) {
      return true;
    }
  }

  return false;
}


bool
  setSceneStateValue(
    SceneState &scene_state,
    const TreePath &path,
    NumericValue value,
    const TreePaths &tree_paths
  )
{
  for (auto body_index : indicesOf(tree_paths.bodies)) {
    const TreePaths::Body &body_paths = tree_paths.bodies[body_index];

    if (startsWith(path, body_paths.path)) {
      SceneState::Body &body_state = scene_state.body(body_index);
      TransformState transform_state = body_state.transform;

      if (setTransformValue(transform_state, path, value, body_paths)) {
        body_state.transform = transform_state;
        return true;
      }

      if (startsWith(path, body_paths.geometry.path)) {
        SceneState::XYZ &scale = body_state.scale;
        Eigen::Vector3f v = {scale.x, scale.y, scale.z};
        const TreePaths::XYZ& xyz_path = body_paths.geometry.scale;
        setVectorValue(v, path, value, xyz_path);
        scale.x = v.x();
        scale.y = v.y();
        scale.z = v.z();
        return true;
      }
    }

  }

  {
    bool was_markers_value =
      setMarkersValue(
        path,
        value,
        tree_paths.markers,
        scene_state
      );

    if (was_markers_value) {
      return true;
    }
  }

  {
    bool was_distance_error_value =
      setDistanceErrorsValue(
        scene_state.distance_errors,
        path,
        value,
        tree_paths.distance_errors
      );

    if (was_distance_error_value) {
      return true;
    }
  }

  return false;
}
