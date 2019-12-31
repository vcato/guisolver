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
static int defaultDigitsOfPrecision() { return 2; }


namespace {
struct ItemAdder {
  const TreePath &parent_path;
  TreeWidget &tree_widget;
  int n_children = 0;

  TreePath
    addNumeric(
      const string &label,
      NumericValue value,
      NumericValue minimum_value,
      int digits_of_precision = defaultDigitsOfPrecision()
    )
  {
    TreePath child_path = childPath(parent_path, n_children);

    tree_widget.createNumericItem(
      child_path,
      LabelProperties{label},
      value,
      minimum_value,
      noMaximumNumericValue(),
      digits_of_precision
    );

    ++n_children;
    return child_path;
  }

  TreePath addVoid(const string &label)
  {
    TreePath child_path = childPath(parent_path, n_children);
    tree_widget.createVoidItem(child_path, LabelProperties{label});
    ++n_children;
    return child_path;
  }

  TreePath
  addEnumeration(
    const string &label,
    const TreeWidget::EnumerationOptions &options,
    int value
  )
  {
    TreePath child_path = childPath(parent_path,n_children);

    tree_widget.createEnumerationItem(
      child_path, LabelProperties{label}, options, value
    );

    ++n_children;
    return child_path;
  }
};
}


namespace {
struct AddNumericItemFunction {
  ItemAdder &adder;
  NumericValue minimum_value = noMinimumNumericValue();
  int digits_of_precision = defaultDigitsOfPrecision();

  AddNumericItemFunction(ItemAdder &adder)
  : adder(adder)
  {
  }

  TreePath operator()(const string &label, NumericValue value)
  {
    return adder.addNumeric(label, value, minimum_value, digits_of_precision);
  }
};
}


static TreePaths::XYZ
  createXYZChildren(
    AddNumericItemFunction &add,
    const SceneState::XYZ &value,
    int digits_of_precision = defaultDigitsOfPrecision()
  )
{
  TreePaths::XYZ xyz_paths;
  xyz_paths.path = add.adder.parent_path;
  add.digits_of_precision = digits_of_precision;

  xyz_paths.x = add("x:", value.x);
  xyz_paths.y = add("y:", value.y);
  xyz_paths.z = add("z:", value.z);

  return xyz_paths;
}


static TreePaths::XYZ
  createXYZChildren(
    TreeWidget &tree_widget,
    const TreePath &parent_path,
    const SceneState::XYZ &value,
    int digits_of_precision = defaultDigitsOfPrecision()
  )
{
  ItemAdder adder{parent_path, tree_widget};
  AddNumericItemFunction add(adder);
  return createXYZChildren(add, value, digits_of_precision);
}


static TreePaths::Marker
  createMarker(
    TreeWidget &tree_widget,
    const TreePath &path,
    const string &name,
    const SceneState::XYZ &position
  )
{
  tree_widget.createVoidItem(path,LabelProperties{"[Marker] " + name});
  ItemAdder adder{path, tree_widget};
  adder.addVoid("name: \"" + name + "\"");
  TreePath position_path = adder.addVoid("position: []");

  TreePaths::Position position_paths =
    TreePaths::Position(
      createXYZChildren(tree_widget, position_path, position)
    );

  TreePaths::Marker marker_paths = {path, position_paths};
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


static string
markerName(
  const Optional<MarkerIndex> &maybe_marker_index,
  const SceneState::Markers &state_markers
)
{
  string result = "";

  if (maybe_marker_index) {
    result = state_markers[*maybe_marker_index].name;
  }

  return result;
}


static string
distanceErrorLabel(
  const SceneState::DistanceError &distance_error_state,
  const SceneState::Markers &marker_states
)
{
  string label = "[DistanceError]";

  Optional<MarkerIndex> optional_start_index =
    distance_error_state.optional_start_marker_index;

  Optional<MarkerIndex> optional_end_index =
    distance_error_state.optional_end_marker_index;

  string start_marker_name = markerName(optional_start_index, marker_states);
  string end_marker_name = markerName(optional_end_index, marker_states);

  if (start_marker_name != "" && end_marker_name != "") {
    label += " " + start_marker_name + " <-> " + end_marker_name;
  }
  else if (start_marker_name != "") {
    label += " " + start_marker_name;
  }
  else if (end_marker_name != "") {
    label += " " + end_marker_name;
  }

  return label;
}


static TreePaths::DistanceError
createDistanceErrorInTree1(
  TreeWidget &tree_widget,
  TreePaths &/*tree_paths*/,
  const TreePath &path,
  const SceneState::Markers &marker_states,
  const SceneState::DistanceError &distance_error_state
)
{
  Optional<MarkerIndex> optional_start_index =
    distance_error_state.optional_start_marker_index;

  Optional<MarkerIndex> optional_end_index =
    distance_error_state.optional_end_marker_index;

  string label = distanceErrorLabel(distance_error_state, marker_states);

  tree_widget.createVoidItem(path,LabelProperties{label});

  TreeWidget::EnumerationOptions marker_options =
    markerEnumerationOptions(marker_states);

  ItemAdder adder{path, tree_widget};

  TreePath start_path =
    adder.addEnumeration(
      "start:",
      marker_options,
      enumerationValueFromMarkerIndex(optional_start_index)
    );

  TreePath end_path =
    adder.addEnumeration(
      "end:",
      marker_options,
      enumerationValueFromMarkerIndex(optional_end_index)
    );

  TreePath distance_path = adder.addVoid("distance:");

  TreePath desired_distance_path =
    adder.addNumeric("desired_distance:", 0, /*minimum_value*/0);

  TreePath weight_path =
    adder.addNumeric("weight:", 1, /*minimum_value*/0);

  TreePath error_path = adder.addVoid("error:");

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


static int
nChildBodies(
  const Optional<BodyIndex> &maybe_parent_index,
  const TreePaths &tree_paths,
  const SceneState &scene_state
)
{
  int n_child_bodies = 0;

  for (BodyIndex body_index : indicesOf(tree_paths.bodies)) {
    if (scene_state.body(body_index).maybe_parent_index == maybe_parent_index) {
      ++n_child_bodies;
    }
  }

  return n_child_bodies;
}


static int
nMarkersOn(
  Optional<BodyIndex> maybe_body_index,
  const TreePaths &tree_paths,
  const SceneState &scene_state
)
{
  int n_attached_markers = 0;

  for (MarkerIndex marker_index : indicesOf(tree_paths.markers)) {
    if (scene_state.marker(marker_index).maybe_body_index == maybe_body_index) {
      ++n_attached_markers;
    }
  }

  return n_attached_markers;
}


namespace {
struct NextPaths {
  TreePath body_path;
  TreePath marker_path;
  TreePath distance_error_path;
};
}


static TreePath
bodyPath(Optional<BodyIndex> maybe_body_index, const TreePaths &tree_paths)
{
  if (maybe_body_index) {
    const TreePaths::Body &body_paths = tree_paths.bodies[*maybe_body_index];
    return body_paths.path;
  }
  else {
    return tree_paths.path;
  }
}


static int
nDistanceErrorsOn(
  Optional<BodyIndex> maybe_body_index,
  const TreePaths &tree_paths,
  const SceneState &/*scene_state*/
)
{
  if (!maybe_body_index) {
    return tree_paths.distance_errors.size();
  }
  else {
    return 0;
  }
}


static NextPaths
nextPaths(
  Optional<BodyIndex> maybe_body_index,
  const TreePaths &tree_paths,
  const SceneState &scene_state
)
{
  NextPaths result;
  int n_bodies = nChildBodies(maybe_body_index, tree_paths, scene_state);
  int n_markers = nMarkersOn(maybe_body_index, tree_paths, scene_state);

  int n_distance_errors =
    nDistanceErrorsOn(maybe_body_index, tree_paths, scene_state);

  TreeItemIndex index = 0;

  if (maybe_body_index) {
    index += 3;
        // 3 for translation, rotation, and geometry
  }

  const TreePath body_path = bodyPath(maybe_body_index, tree_paths);
  index += n_markers;
  result.marker_path = childPath(body_path, index);
  index += n_bodies;
  result.body_path = childPath(body_path, index);
  index += n_distance_errors;
  result.distance_error_path = childPath(body_path, index);
  return result;
}


static TreePath
nextBodyPath(
  Optional<BodyIndex> maybe_parent_index,
  const TreePaths &tree_paths,
  const SceneState &scene_state
)
{
  return nextPaths(maybe_parent_index, tree_paths, scene_state).body_path;
}


static TreePath
nextMarkerPath(
  Optional<BodyIndex> maybe_body_index,
  const TreePaths &tree_paths,
  const SceneState &scene_state
)
{
  return nextPaths(maybe_body_index, tree_paths, scene_state).marker_path;
}


static TreePath
nextDistanceErrorPath(
  const TreePaths &tree_paths,
  const SceneState &scene_state
)
{
  return nextPaths({}, tree_paths, scene_state).distance_error_path;
}


void
  createDistanceErrorInTree(
    const SceneState::DistanceError &state_distance_error,
    TreeWidget &tree_widget,
    TreePaths &tree_paths,
    const SceneState &scene_state
  )
{
  const TreePath next_distance_error_path =
    nextDistanceErrorPath(tree_paths, scene_state);

  TreePath distance_error_path = next_distance_error_path;
  handlePathInsertion(tree_paths, distance_error_path);

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
  DistanceErrorIndex distance_error_index,
  TreePaths &tree_paths,
  TreeWidget &tree_widget
)
{
  TreePaths::DistanceErrors &distance_errors = tree_paths.distance_errors;
  TreePath distance_error_path = distance_errors[distance_error_index].path;
  tree_widget.removeItem(distance_error_path);
  removeIndexFrom(distance_errors, distance_error_index);
  handlePathRemoval(tree_paths, distance_error_path);
}


void
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
}


static void
  updateDistanceError(
    DistanceErrorIndex distance_error_index,
    TreeWidget &tree_widget,
    const TreePaths &tree_paths,
    const SceneState &scene_state
  )
{
  const TreePaths::DistanceError &distance_error_paths =
    tree_paths.distance_errors[distance_error_index];

  const SceneState::DistanceError &distance_error_state =
    scene_state.distance_errors[distance_error_index];

  tree_widget.setItemLabel(
    distance_error_paths.path,
    distanceErrorLabel(distance_error_state, scene_state.markers())
  );

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
  Optional<BodyIndex> maybe_body_index = state_marker.maybe_body_index;

  const TreePath insert_point =
    nextMarkerPath(maybe_body_index, tree_paths, scene_state);

  TreePath marker_path = insert_point;

  handlePathInsertion(tree_paths, marker_path);

  tree_paths.markers.push_back(
    createMarker(
      tree_widget, marker_path, state_marker.name, state_marker.position
    )
  );
}


static TreePaths::XYZ
addXYZ(ItemAdder &adder, const string &label, const SceneState::XYZ &xyz)
{
  TreePath path = adder.addVoid(label);
  return createXYZChildren(adder.tree_widget, path, xyz);
}


static TreePaths::Body
createBodyItem(
  const TreePath &body_path,
  const SceneState::Body &body_state,
  TreeWidget &tree_widget
)
{
  TreePaths::Body body_paths;

  tree_widget.createVoidItem(body_path, LabelProperties{"[Body]"});

  ItemAdder adder{body_path, tree_widget};

  TreePath translation_path = adder.addVoid("translation: []");
  TreePath rotation_path    = adder.addVoid("rotation: []");
  TreePath geometry_path    = adder.addVoid("[Box]");

  TreePath next_body_path = childPath(body_path, adder.n_children);
  TreePath next_marker_path = childPath(body_path, adder.n_children);
  body_paths.path = body_path;

  body_paths.translation =
    TreePaths::Translation(
      createXYZChildren(
        tree_widget,
        translation_path,
        body_state.transform.translation
      )
    );

  body_paths.rotation =
    TreePaths::Rotation(
      createXYZChildren(
        tree_widget,
        rotation_path,
        body_state.transform.rotation,
        /*digits_of_precision*/1
      )
    );

  body_paths.geometry.path = geometry_path;

  {
    ItemAdder adder{geometry_path, tree_widget};
    TreePaths::Body::Geometry &geometry_paths = body_paths.geometry;
    const SceneState::Geometry &geometry_state = body_state.geometry;

    {
      string label = "scale: []";
      TreePath path = adder.addVoid(label);
      ItemAdder child_adder{path, adder.tree_widget};
      AddNumericItemFunction add(child_adder);
      add.minimum_value = 0;
      TreePaths::XYZ xyz_paths = createXYZChildren(add, geometry_state.scale);
      geometry_paths.scale = TreePaths::Scale(xyz_paths);
    }

    geometry_paths.center = addXYZ(adder, "center: []", geometry_state.center);
  }

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
  const TreePath body_path = nextBodyPath({}, tree_paths, scene_state);
  assert(BodyIndex(tree_paths.bodies.size()) == body_index);
  const SceneState::Body &body_state = scene_state.body(body_index);

  handlePathInsertion(tree_paths, body_path);

  tree_paths.bodies.push_back(
    createBodyItem(body_path, body_state, tree_widget)
  );
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
    nextBodyPath(parent_body_index, tree_paths, scene_state);

  handlePathInsertion(tree_paths, body_path);

  assert(BodyIndex(tree_paths.bodies.size()) == body_index);
  const SceneState::Body &body_state = scene_state.body(body_index);

  tree_paths.bodies.push_back(
    createBodyItem(body_path, body_state, tree_widget)
  );
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

  for (auto marker_index : indicesOfMarkersOnBody(body_index, scene_state)) {
    createMarkerInTree(tree_widget, tree_paths, scene_state, marker_index);
  }

  for (auto child_body_index : indicesOfChildBodies(body_index, scene_state)) {
    createBodyInTree(tree_widget, tree_paths, scene_state, child_body_index);
  }
}


void
removeBodyFromTree(
  TreeWidget &tree_widget,
  TreePaths &tree_paths,
  const SceneState &scene_state,
  BodyIndex body_index
)
{
  assert(indicesOfMarkersOnBody(body_index, scene_state).empty());
  assert(indicesOfChildBodies(body_index, scene_state).empty());

  TreePath body_path = tree_paths.bodies[body_index].path;
  tree_widget.removeItem(body_path);
  removeIndexFrom(tree_paths.bodies, body_index);
  handlePathRemoval(tree_paths, body_path);
}


TreePaths fillTree(TreeWidget &tree_widget, const SceneState &scene_state)
{
  TreePaths tree_paths;
  TreePath scene_path = {0};
  tree_widget.createVoidItem(scene_path,LabelProperties{"[Scene]"});
  tree_paths.path = scene_path;

  TreePath next_scene_child_path = childPath(scene_path, 0);
  tree_paths.total_error = next_scene_child_path;

  tree_widget.createVoidItem(
    tree_paths.total_error, LabelProperties{totalErrorLabel(0)}
  );

  for (auto body_index : indicesOfChildBodies({}, scene_state)) {
    createBodyInTree(tree_widget, tree_paths, scene_state, body_index);
  }

  for (auto marker_index : indicesOfMarkersOnBody({}, scene_state)) {
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
  const SceneState::Body &body_state = state.body(body_index);
  const TransformState &global = body_state.transform;
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
  {
    const TreePaths::XYZ &scale_paths = body_paths.geometry.scale;
    const SceneState::XYZ &scale = body_state.geometry.scale;
    updateXYZValues(tree_widget, scale_paths, vec3(scale));
  }
  {
    const TreePaths::XYZ &center_paths = body_paths.geometry.center;
    const SceneState::XYZ &center = body_state.geometry.center;
    updateXYZValues(tree_widget, center_paths, vec3(center));
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
    updateDistanceError(i, tree_widget, tree_paths, state);
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

    tree_widget.setItemLabel(
      distance_error_paths.path,
      distanceErrorLabel(distance_error_state, scene_state.markers())
    );
  }
}


static bool
  setVectorValue(
    SceneState::XYZ &v,
    const TreePath &path,
    NumericValue value,
    const TreePaths::XYZ &xyz_path
  )
{
  if (path == xyz_path.x) {
    v.x = value;
    return true;
  }

  if (path == xyz_path.y) {
    v.y = value;
    return true;
  }

  if (path == xyz_path.z) {
    v.z = value;
    return true;
  }

  assert(false); // not implemented
  return false;
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
    return
      setVectorValue(marker_state.position, path, value, marker_path.position);
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
        SceneState::Geometry &geometry_state =
          body_state.geometry;

        const TreePaths::Body::Geometry &geometry_paths =
          body_paths.geometry;

        if (startsWith(path, geometry_paths.scale.path)) {
          return
            setVectorValue(
              geometry_state.scale, path, value, geometry_paths.scale
            );
        }
        else if (startsWith(path, geometry_paths.center.path)) {
          return
            setVectorValue(
              geometry_state.center, path, value, geometry_paths.center
            );
        }
        else {
          assert(false); // not implemented
        }
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
