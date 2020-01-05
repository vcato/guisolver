#include "treevalues.hpp"

#include "rotationvector.hpp"
#include "numericvalue.hpp"
#include "stringvalue.hpp"
#include "indicesof.hpp"
#include "vectorio.hpp"
#include "removeindexfrom.hpp"
#include "startswith.hpp"
#include "numericvaluelimits.hpp"
#include "transformstate.hpp"

using std::cerr;
using std::string;
using LabelProperties = TreeWidget::LabelProperties;
using BoxPaths = TreePaths::Box;
using LinePaths = TreePaths::Line;
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

  TreePath addString(const string &label, const StringValue &value)
  {
    TreePath child_path = childPath(parent_path, n_children);
    tree_widget.createStringItem(child_path, LabelProperties{label}, value);
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


static string markerLabel(const SceneState::Marker &marker_state)
{
  return "[Marker] " + marker_state.name;
}


static string bodyLabel(const SceneState::Body &body_state)
{
  return "[Body] " + body_state.name;
}


static TreePaths::Marker
createMarker(
  TreeWidget &tree_widget,
  const TreePath &path,
  const SceneState::Marker &state_marker
)
{
  const string &name = state_marker.name;
  const SceneState::XYZ &position = state_marker.position;
  tree_widget.createVoidItem(path,LabelProperties{markerLabel(state_marker)});
  ItemAdder adder{path, tree_widget};
  TreePath name_path = adder.addString("name:", name);
  TreePath position_path = adder.addVoid("position: []");

  TreePaths::Position position_paths =
    TreePaths::Position(
      createXYZChildren(tree_widget, position_path, position)
    );

  TreePaths::Marker marker_paths = {path, name_path, position_paths};
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


static string
errorLabel(
  const SceneState::DistanceError &distance_error_state
)
{
  std::ostringstream label_stream;
  label_stream << "error: " << distance_error_state.error;
  string label_string = label_stream.str();
  return label_string;
}


static string
distanceLabel(
  const SceneState::DistanceError &distance_error_state
)
{
  std::ostringstream label_stream;

  if (distance_error_state.maybe_distance) {
    label_stream << "distance: " << *distance_error_state.maybe_distance;
  }
  else {
    label_stream << "distance: N/A";
  }

  string label_string = label_stream.str();
  return label_string;
}


static TreePaths::DistanceError
createDistanceErrorInTree1(
  TreeWidget &tree_widget,
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

  TreePath distance_path = adder.addVoid(distanceLabel(distance_error_state));

  TreePath desired_distance_path =
    adder.addNumeric(
      "desired_distance:",
      distance_error_state.desired_distance,
      /*minimum_value*/0
    );

  TreePath weight_path =
    adder.addNumeric(
      "weight:",
      distance_error_state.weight,
      /*minimum_value*/0
    );

  TreePath error_path = adder.addVoid(errorLabel(distance_error_state));

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

      assert(path_to_update[depth] != path_removed[depth]);

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
static void visitPaths(vector<Optional<T>> &v, const Visitor &visitor)
{
  for (auto i : indicesOf(v)) {
    if (v[i]) {
      visitPaths(*v[i], visitor);
    }
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


template <typename Paths>
static void
handlePathInsertion(Paths &paths, const TreePath &path_to_insert)
{
  visitPaths(
    paths,
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
    if (tree_paths.bodies[body_index].hasValue()) {
      if (
        scene_state.body(body_index).maybe_parent_index == maybe_parent_index
      ) {
        ++n_child_bodies;
      }
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
    if (tree_paths.markers[marker_index].hasValue()) {
      if (
        scene_state.marker(marker_index).maybe_body_index == maybe_body_index
      ) {
        ++n_attached_markers;
      }
    }
  }

  return n_attached_markers;
}


namespace {
struct NextPaths {
  TreePath box_path;
  TreePath line_path;
  TreePath body_path;
  TreePath marker_path;
  TreePath distance_error_path;
};
}


static TreePath
bodyPath(Optional<BodyIndex> maybe_body_index, const TreePaths &tree_paths)
{
  if (maybe_body_index) {
    const TreePaths::Body &body_paths = tree_paths.body(*maybe_body_index);
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

  const TreePath body_path = bodyPath(maybe_body_index, tree_paths);
  TreeItemIndex index = 0;

  if (maybe_body_index) {
    int n_boxes = tree_paths.body(*maybe_body_index).boxes.size();
    index += 3; // 3 for name, translation, rotation
    index += n_boxes;
  }
  else {
    // Can't add boxes to the scene
  }

  result.box_path = childPath(body_path, index);

  if (maybe_body_index) {
    int n_lines = tree_paths.body(*maybe_body_index).lines.size();
    index += n_lines;
    result.line_path = childPath(body_path, index);
  }
  else {
    // Can't add lines to the scene
  }

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
removeMarkerItemFromTree(
  MarkerIndex marker_index,
  TreeWidget &tree_widget,
  TreePaths &tree_paths
)
{
  TreePath marker_path = tree_paths.marker(marker_index).path;
  tree_widget.removeItem(marker_path);
  tree_paths.markers[marker_index].reset();
  handlePathRemoval(tree_paths, marker_path);
}


void
removeMarkerFromTree(
  MarkerIndex marker_index,
  TreePaths &tree_paths,
  TreeWidget &tree_widget
)
{
  removeMarkerItemFromTree(marker_index, tree_widget, tree_paths);
  removeIndexFrom(tree_paths.markers, marker_index);
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
    string label_string = distanceLabel(distance_error_state);
    tree_widget.setItemLabel(distance_error_paths.distance, label_string);
  }
  {
    string label_string = errorLabel(distance_error_state);
    tree_widget.setItemLabel(distance_error_paths.error, label_string);
  }
}


static string totalErrorLabel(float total_error)
{
  std::ostringstream label_stream;
  label_stream << "total_error: " << total_error;
  return label_stream.str();
}


void
createMarkerItemInTree(
  MarkerIndex marker_index,
  TreeWidget &tree_widget,
  TreePaths &tree_paths,
  const SceneState &scene_state
)
{
  const SceneState::Marker &state_marker = scene_state.marker(marker_index);
  Optional<BodyIndex> maybe_body_index = state_marker.maybe_body_index;

  const TreePath marker_path =
    nextMarkerPath(maybe_body_index, tree_paths, scene_state);

  handlePathInsertion(tree_paths, marker_path);

  tree_paths.markers[marker_index] =
    createMarker(tree_widget, marker_path, state_marker);
}


void
createMarkerInTree(
  TreeWidget &tree_widget,
  TreePaths &tree_paths,
  const SceneState &scene_state,
  MarkerIndex marker_index
)
{
  assert(marker_index == MarkerIndex(tree_paths.markers.size()));
  tree_paths.markers.emplace_back();

  createMarkerItemInTree(
    marker_index,
    tree_widget,
    tree_paths,
    scene_state
  );
}


static TreePaths::XYZ
addXYZ(ItemAdder &adder, const string &label, const SceneState::XYZ &xyz)
{
  TreePath path = adder.addVoid(label);
  return createXYZChildren(adder.tree_widget, path, xyz);
}


static BoxPaths
createBoxItem(
  const TreePath &box_path,
  TreeWidget &tree_widget,
  const SceneState::Box &box_state
)
{
  BoxPaths box_paths;
  tree_widget.createVoidItem(box_path, LabelProperties{"[Box]"});
  box_paths.path = box_path;
  ItemAdder adder{box_path, tree_widget};

  {
    TreePath path = adder.addVoid("scale: []");
    ItemAdder child_adder{path, adder.tree_widget};
    AddNumericItemFunction add(child_adder);
    add.minimum_value = 0;
    TreePaths::XYZ xyz_paths = createXYZChildren(add, box_state.scale);
    box_paths.scale = TreePaths::Scale(xyz_paths);
  }

  box_paths.center = addXYZ(adder, "center: []", box_state.center);
  return box_paths;
}


static LinePaths
createLineItem(
  const TreePath &line_path,
  TreeWidget &tree_widget,
  const SceneState::Line &line_state
)
{
  LinePaths line_paths;
  tree_widget.createVoidItem(line_path, LabelProperties{"[Line]"});
  line_paths.path = line_path;
  ItemAdder adder{line_path, tree_widget};
  line_paths.start = addXYZ(adder, "start: []", line_state.start);
  line_paths.end = addXYZ(adder, "end: []", line_state.end);
  return line_paths;
}


static TreePaths::Body
createBodyItem(
  const TreePath &body_path,
  const SceneState::Body &body_state,
  TreeWidget &tree_widget
)
{
  TreePaths::Body body_paths;
  tree_widget.createVoidItem(body_path, LabelProperties{bodyLabel(body_state)});
  ItemAdder adder{body_path, tree_widget};

  TreePath name_path        = adder.addString("name:",body_state.name);
  TreePath translation_path = adder.addVoid("translation: []");
  TreePath rotation_path    = adder.addVoid("rotation: []");

  body_paths.path = body_path;
  body_paths.name = name_path;

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

  size_t n_boxes = body_state.boxes.size();
  body_paths.boxes.resize(n_boxes);

  for (size_t i=0; i!=n_boxes; ++i) {
    TreePath box_path = childPath(adder.parent_path, adder.n_children);

    body_paths.boxes[i] =
      createBoxItem(box_path, tree_widget, body_state.boxes[i]);

    ++adder.n_children;
  }

  return body_paths;
}


static void
createBodyItemInTree(
  BodyIndex body_index,
  TreeWidget &tree_widget,
  TreePaths &tree_paths,
  const SceneState &scene_state
)
{
  const SceneState::Body &state_body = scene_state.body(body_index);
  const Optional<BodyIndex> &maybe_parent_index = state_body.maybe_parent_index;

  const TreePath body_path =
    nextBodyPath(maybe_parent_index, tree_paths, scene_state);

  const SceneState::Body &body_state = scene_state.body(body_index);

  handlePathInsertion(tree_paths, body_path);

  tree_paths.bodies[body_index] =
    createBodyItem(body_path, body_state, tree_widget);
}



void
createBoxInTree(
  TreeWidget &tree_widget,
  TreePaths &tree_paths,
  const SceneState &scene_state,
  BodyIndex body_index,
  BoxIndex box_index
)
{
  TreePaths::Body &body_paths = tree_paths.body(body_index);
  const SceneState::Body &body_state = scene_state.body(body_index);
  TreePath box_path = nextPaths(body_index, tree_paths, scene_state).box_path;
  handlePathInsertion(tree_paths, box_path);

  assert(box_index == BoxIndex(body_paths.boxes.size()));
  body_paths.boxes.emplace_back();

  body_paths.boxes[box_index] =
    createBoxItem(box_path, tree_widget, body_state.boxes[box_index]);
}


void
createLineInTree(
  TreeWidget &tree_widget,
  TreePaths &tree_paths,
  const SceneState &scene_state,
  BodyIndex body_index,
  LineIndex line_index
)
{
  TreePaths::Body &body_paths = tree_paths.body(body_index);
  const SceneState::Body &body_state = scene_state.body(body_index);
  TreePath line_path = nextPaths(body_index, tree_paths, scene_state).line_path;
  handlePathInsertion(tree_paths, line_path);

  assert(line_index == LineIndex(body_paths.lines.size()));
  body_paths.lines.emplace_back();

  body_paths.lines[line_index] =
    createLineItem(line_path, tree_widget, body_state.lines[line_index]);
}


// This function creates the body item and all child items and stores
// the path in the tree_paths at the proper index.  It does not shift
// any indices.
void
createBodyInTree(
  TreeWidget &tree_widget,
  TreePaths &tree_paths,
  const SceneState &scene_state,
  BodyIndex body_index
)
{
  if (body_index >= BodyIndex(tree_paths.bodies.size())) {
    tree_paths.bodies.resize(body_index+1);
  }

  assert(!tree_paths.bodies[body_index]);
  createBodyItemInTree(body_index, tree_widget, tree_paths, scene_state);

  for (auto marker_index : indicesOfMarkersOnBody(body_index, scene_state)) {
    createMarkerInTree(tree_widget, tree_paths, scene_state, marker_index);
  }

  for (auto child_body_index : indicesOfChildBodies(body_index, scene_state)) {
    createBodyInTree(tree_widget, tree_paths, scene_state, child_body_index);
  }
}


void
removeBodyItemFromTree(
  BodyIndex body_index,
  TreeWidget &tree_widget,
  TreePaths &tree_paths
)
{
  TreePath body_path = tree_paths.body(body_index).path;
  tree_widget.removeItem(body_path);
  tree_paths.bodies[body_index].reset();
  handlePathRemoval(tree_paths, body_path);
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
  removeBodyItemFromTree(body_index, tree_widget, tree_paths);
  removeIndexFrom(tree_paths.bodies, body_index);
}


void
removeBoxFromTree(
  TreeWidget &tree_widget,
  TreePaths &tree_paths,
  const SceneState &,
  BodyIndex body_index,
  BoxIndex box_index
)
{
  TreePath box_path = tree_paths.body(body_index).boxes[box_index].path;
  tree_widget.removeItem(box_path);
  removeIndexFrom(tree_paths.bodies[body_index]->boxes, box_index);
  handlePathRemoval(tree_paths, box_path);
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
    const SceneState::Markers &markers,
    const TreePaths &tree_paths
  )
{
  tree_widget.setItemLabel(
    tree_paths.marker(i).path,
    markerLabel(markers[i])
  );

  updateXYZValues(
    tree_widget,
    tree_paths.marker(i).position,
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
  const TreePaths::Body &body_paths = tree_paths.body(body_index);

  tree_widget.setItemLabel(
    tree_paths.body(body_index).path,
    bodyLabel(body_state)
  );

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
  assert(body_paths.boxes.size() == body_state.boxes.size());
  size_t n_boxes = body_state.boxes.size();

  for (size_t i=0; i!=n_boxes; ++i) {
    const BoxPaths &box_paths = body_paths.boxes[i];
    const SceneState::Box &box_state = body_state.boxes[i];
    {
      const TreePaths::XYZ &scale_paths = box_paths.scale;
      const SceneState::XYZ &scale = box_state.scale;
      updateXYZValues(tree_widget, scale_paths, vec3(scale));
    }
    {
      const TreePaths::XYZ &center_paths = box_paths.center;
      const SceneState::XYZ &center = box_state.center;
      updateXYZValues(tree_widget, center_paths, vec3(center));
    }
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
    updateMarker(i, tree_widget, state.markers(), tree_paths);
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
  setMarkerNumericValue(
    const TreePath &path,
    NumericValue value,
    const TreePaths::Marker &marker_path,
    SceneState::Marker &marker_state
  )
{
  if (startsWith(path, marker_path.position.path)) {
    return
      setVectorValue(marker_state.position, path, value, marker_path.position);
  }

  return false;
}


static bool
  setMarkerStringValue(
    const TreePath &path,
    const StringValue &value,
    SceneState &scene_state,
    const TreePaths &tree_paths,
    MarkerIndex marker_index
  )
{
  const TreePaths::Marker &marker_path = tree_paths.marker(marker_index);
  SceneState::Marker &marker_state = scene_state.marker(marker_index);

  if (startsWith(path, marker_path.name)) {
    if (findMarkerIndex(scene_state, value)) {
      // Name already exists.
      return false;
    }

    marker_state.name = value;
    return true;
  }

  return false;
}


static bool
  setBodyStringValue(
    const TreePath &path,
    const StringValue &value,
    SceneState &scene_state,
    const TreePaths &tree_paths,
    BodyIndex body_index
  )
{
  const TreePaths::Body &body_paths = tree_paths.body(body_index);
  SceneState::Body &body_state = scene_state.body(body_index);

  if (startsWith(path, body_paths.name)) {
    if (findBodyIndex(scene_state, value)) {
      // Name already exists.
      return false;
    }

    body_state.name = value;
    return true;
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


static bool
setBoxNumericValue(
  SceneState::Box &box_state,
  const TreePath &path,
  NumericValue value,
  const TreePaths::Box &box_paths
)
{
  if (startsWith(path, box_paths.path)) {
    if (startsWith(path, box_paths.scale.path)) {
      return setVectorValue(box_state.scale, path, value, box_paths.scale);
    }
    else if (startsWith(path, box_paths.center.path)) {
      return
        setVectorValue(box_state.center, path, value, box_paths.center);
    }
    else {
      assert(false); // not implemented
    }
  }

  return false;
}


static bool
setLineNumericValue(
  SceneState::Line &line_state,
  const TreePath &path,
  NumericValue value,
  const TreePaths::Line &line_paths
)
{
  if (startsWith(path, line_paths.path)) {
    if (startsWith(path, line_paths.start.path)) {
      return setVectorValue(line_state.start, path, value, line_paths.start);
    }

    if (startsWith(path, line_paths.end.path)) {
      return setVectorValue(line_state.end, path, value, line_paths.end);
    }
  }

  return false;
}


static bool
setBodyNumericValue(
  SceneState::Body &body_state,
  const TreePath &path,
  NumericValue value,
  const TreePaths::Body &body_paths
)
{
  if (startsWith(path, body_paths.path)) {
    TransformState transform_state = body_state.transform;

    if (setTransformValue(transform_state, path, value, body_paths)) {
      body_state.transform = transform_state;
      return true;
    }

    assert(body_paths.boxes.size() == body_state.boxes.size());
    size_t n_boxes = body_state.boxes.size();

    for (size_t box_index = 0; box_index != n_boxes; ++box_index) {
      const BoxPaths &box_paths = body_paths.boxes[box_index];
      SceneState::Box &box_state = body_state.boxes[box_index];

      if (setBoxNumericValue(box_state, path, value, box_paths)) {
        return true;
      }
    }

    LineIndex n_lines = body_state.lines.size();

    for (LineIndex line_index = 0; line_index != n_lines; ++line_index) {
      const LinePaths &line_paths = body_paths.lines[line_index];
      SceneState::Line &line_state = body_state.lines[line_index];

      if (setLineNumericValue(line_state, path, value, line_paths)) {
        return true;
      }
    }
  }

  return false;
}


bool
setSceneStateNumericValue(
  SceneState &scene_state,
  const TreePath &path,
  NumericValue value,
  const TreePaths &tree_paths
)
{
  for (auto body_index : indicesOf(tree_paths.bodies)) {
    const TreePaths::Body &body_paths = tree_paths.body(body_index);
    SceneState::Body &body_state = scene_state.body(body_index);

    if (setBodyNumericValue(body_state, path, value, body_paths)) {
      return true;
    }
  }

  for (auto i : indicesOf(tree_paths.markers)) {
    bool value_was_set =
      setMarkerNumericValue(
        path,
        value,
        tree_paths.marker(i),
        scene_state.marker(i)
      );

    if (value_was_set) {
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


bool
setSceneStateStringValue(
  SceneState &scene_state,
  const TreePath &path,
  const StringValue &value,
  const TreePaths &tree_paths
)
{
  for (auto i : indicesOf(tree_paths.bodies)) {
    bool value_was_set =
      setBodyStringValue(path, value, scene_state, tree_paths, i);

    if (value_was_set) {
      return true;
    }
  }

  for (auto i : indicesOf(tree_paths.markers)) {
    bool value_was_set =
      setMarkerStringValue(path, value, scene_state, tree_paths, i);

    if (value_was_set) {
      return true;
    }
  }

  return false;
}


void
removeBodyBranchItemsFromTree(
  BodyIndex body_index,
  TreeWidget &tree_widget,
  TreePaths &tree_paths,
  const SceneState &scene_state
)
{
  struct Visitor {
    TreeWidget &tree_widget;
    TreePaths &tree_paths;

    void visitBody(BodyIndex body_index) const
    {
      removeBodyItemFromTree(body_index, tree_widget, tree_paths);
    }

    void visitMarker(MarkerIndex marker_index) const
    {
      removeMarkerItemFromTree(marker_index, tree_widget, tree_paths);
    }
  } visitor = {tree_widget, tree_paths};

  forEachBranchIndexInPostOrder(body_index, scene_state, visitor);
}


void
createBodyBranchItemsInTree(
  BodyIndex body_index,
  TreeWidget &tree_widget,
  TreePaths &tree_paths,
  const SceneState &scene_state
)
{
  struct Visitor {
    TreeWidget &tree_widget;
    TreePaths &tree_paths;
    const SceneState &scene_state;

    void visitBody(BodyIndex body_index) const
    {
      createBodyItemInTree(body_index, tree_widget, tree_paths, scene_state);
    }

    void visitMarker(MarkerIndex marker_index) const
    {
      createMarkerItemInTree(
        marker_index, tree_widget, tree_paths, scene_state
      );
    }
  } visitor = {tree_widget, tree_paths, scene_state};

  forEachBranchIndexInPreOrder(body_index, scene_state, visitor);
}
