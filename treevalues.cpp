#include "treevalues.hpp"

#include <sstream>
#include "vec3.hpp"
#include "numericvalue.hpp"
#include "stringvalue.hpp"
#include "indicesof.hpp"
#include "vectorio.hpp"
#include "removeindexfrom.hpp"
#include "startswith.hpp"
#include "numericvaluelimits.hpp"
#include "vec3state.hpp"
#include "xyzcomponent.hpp"
#include "channel.hpp"
#include "emplaceinto.hpp"
#include "pointlink.hpp"

using std::cerr;
using std::string;
using LabelProperties = TreeWidget::LabelProperties;
using BoxPaths = TreePaths::Box;
using MarkerPaths = TreePaths::Marker;
using LinePaths = TreePaths::Line;
using MeshPaths = TreePaths::Mesh;
using BodyState = SceneState::Body;
using BodyPaths = TreePaths::Body;
using ChannelPaths = TreePaths::Channel;


static int defaultDigitsOfPrecision() { return 2; }


namespace {
struct NumericProperties {
  int digits_of_precision = defaultDigitsOfPrecision();
  NumericValue minimum_value = noMinimumNumericValue();
};
}


namespace {
struct ItemAdder {
  const TreePath &parent_path;
  TreeWidget &tree_widget;
  int n_children = 0;

  TreePath
    addNumeric2(
      const string &label,
      NumericValue value,
      const NumericProperties &properties
    )
  {
    TreePath child_path = childPath(parent_path, n_children);

    tree_widget.createNumericItem(
      child_path,
      LabelProperties{label},
      value,
      properties.minimum_value,
      noMaximumNumericValue(),
      properties.digits_of_precision
    );

    ++n_children;
    return child_path;
  }

  TreePath
    addNumeric(
      const string &label,
      NumericValue value,
      NumericValue minimum_value
    )
  {
    NumericProperties properties = {defaultDigitsOfPrecision(), minimum_value};
    return addNumeric2(label, value, properties);
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

  TreePath addBool(const string &label, bool value)
  {
    TreePath child_path = childPath(parent_path, n_children);
    tree_widget.createBoolItem(child_path, LabelProperties{label}, value);
    ++n_children;
    return child_path;
  }
};
}


namespace {
struct AddNumericItemFunction {
  ItemAdder &adder;
  NumericProperties properties;

  AddNumericItemFunction(ItemAdder &adder)
  : adder(adder)
  {
  }
};
}


namespace {
struct AddSimpleNumericItemFunction : AddNumericItemFunction {
  using ItemPaths = TreePath;

  AddSimpleNumericItemFunction(ItemAdder &adder)
  : AddNumericItemFunction(adder)
  {
  }

  ItemPaths
  operator()(
    const string &label,
    NumericValue value
  )
  {
    return adder.addNumeric2(label, value, properties);
  }
};
}


static ChannelPaths
addChannelItemChildren(
  const TreePath &path,
  TreeWidget &tree_widget,
  Optional<bool> maybe_solve_state,
  const Expression &expression
)
{
  ChannelPaths channel_paths;
  channel_paths.path = path;
  ItemAdder child_adder{path, tree_widget};

  if (maybe_solve_state) {
    channel_paths.maybe_solve_path =
      child_adder.addBool("solve", *maybe_solve_state);
  }

  channel_paths.expression_path =
    child_adder.addString("expression", expression);

  return channel_paths;
}


namespace {
struct AddChannelItemFunction : AddNumericItemFunction {
  using ItemPaths = ChannelPaths;

  AddChannelItemFunction(ItemAdder &adder)
  : AddNumericItemFunction(adder)
  {
  }

  ChannelPaths
  operator()(
    const string &label,
    NumericValue value,
    Optional<bool> maybe_solve_state,
    const Expression &expression
  )
  {
    TreePath path = adder.addNumeric2(label, value, properties);

    return
      addChannelItemChildren(
        path, adder.tree_widget, maybe_solve_state, expression
      );
  }

  ChannelPaths
  operator()(
    const string &label,
    NumericValue value,
    const Expression &expression
  )
  {
    return
      operator()(
        label, value, /*maybe_solve_state*/Optional<bool>(), expression
      );
  }
};
}


template <
  typename AddFunction,
  typename ItemPaths = typename AddFunction::ItemPaths,
  typename XYZPaths = TreePaths::BasicXYZ<ItemPaths>
>
static XYZPaths
createXYZChildren2(
  AddFunction &add,
  const SceneState::XYZ &value,
  int digits_of_precision = defaultDigitsOfPrecision()
)
{
  XYZPaths xyz_paths;
  xyz_paths.path = add.adder.parent_path;
  add.properties.digits_of_precision = digits_of_precision;

  xyz_paths.x = add("x:", value.x);
  xyz_paths.y = add("y:", value.y);
  xyz_paths.z = add("z:", value.z);

  return xyz_paths;
}


template <
  typename AddFunction,
  typename ItemPaths = typename AddFunction::ItemPaths,
  typename XYZPaths = TreePaths::BasicXYZ<ItemPaths>
>
static XYZPaths
createXYZChildren2(
  AddFunction &add,
  SceneState::XYZChannelsRef channels
)
{
  XYZPaths xyz_paths;
  const SceneState::XYZ &values = channels.values;
  const SceneState::XYZExpressions &expressions = channels.expressions;
  xyz_paths.path = add.adder.parent_path;

  xyz_paths.x = add("x:", values.x, expressions.x);
  xyz_paths.y = add("y:", values.y, expressions.y);
  xyz_paths.z = add("z:", values.z, expressions.z);

  return xyz_paths;
}


template <
  typename AddFunction,
  typename ItemPaths = typename AddFunction::ItemPaths,
  typename XYZPaths = TreePaths::BasicXYZ<ChannelPaths>
>
static XYZPaths
createXYZChannels2(
  AddFunction &add,
  SceneState::XYZChannelsRef channels,
  int digits_of_precision = defaultDigitsOfPrecision()
)
{
  XYZPaths xyz_paths;
  const SceneState::XYZ &values = channels.values;
  const SceneState::XYZExpressions &expressions = channels.expressions;
  xyz_paths.path = add.adder.parent_path;
  add.properties.digits_of_precision = digits_of_precision;

  xyz_paths.x = add("x:", values.x, expressions.x);
  xyz_paths.y = add("y:", values.y, expressions.y);
  xyz_paths.z = add("z:", values.z, expressions.z);

  return xyz_paths;
}


template <typename Values>
static TreePaths::XYZ
  createXYZChildren(
    TreeWidget &tree_widget,
    const TreePath &parent_path,
    const Values &value,
    const NumericProperties &properties
  )
{
  ItemAdder adder{parent_path, tree_widget};
  AddSimpleNumericItemFunction add(adder);
  add.properties = properties;
  return createXYZChildren2(add, value);
}


static TreePaths::XYZChannels
createXYZChannels(
  TreeWidget &tree_widget,
  const TreePath &parent_path,
  const SceneState::XYZ &value,
  const SceneState::XYZSolveFlags &solve_flags,
  const SceneState::XYZExpressions &expressions,
  int digits_of_precision
)
{
  ItemAdder adder{parent_path, tree_widget};
  AddChannelItemFunction add(adder);
  TreePaths::XYZChannels xyz_paths;
  xyz_paths.path = add.adder.parent_path;
  add.properties.digits_of_precision = digits_of_precision;

  xyz_paths.x = add("x:", value.x, solve_flags.x, expressions.x);
  xyz_paths.y = add("y:", value.y, solve_flags.y, expressions.y);
  xyz_paths.z = add("z:", value.z, solve_flags.z, expressions.z);

  return xyz_paths;
}


static string
markerLabel(
  const SceneState::Marker &marker_state,
  const SceneState &scene_state,
  Marker marker
)
{
  string result = "[Marker] " + marker_state.name;

  if (scene_state.maybe_marked_marker == marker) {
    result += " (marked)";
  }

  return result;
}


static string bodyLabel(const SceneState::Body &body_state)
{
  return "[Body] " + body_state.name;
}


static string variableLabel(const SceneState::Variable &variable)
{
  return "[Variable] " + variable.name;
}


static TreePaths::XYZChannels
addXYZChannelsTo(
  ItemAdder &adder,
  const string &label,
  SceneState::XYZChannelsRef channels,
  const NumericProperties &properties
)
{
  TreePath path = adder.addVoid(label);
  ItemAdder child_adder{path, adder.tree_widget};
  AddChannelItemFunction add(child_adder);
  add.properties = properties;
  return createXYZChannels2(add, channels);
}


static MarkerPaths
createMarker(
  TreeWidget &tree_widget,
  const TreePath &path,
  const SceneState::Marker &marker_state,
  const SceneState &scene_state,
  Marker marker
)
{
  MarkerPaths marker_paths;
  marker_paths.path = path;
  const string &name = marker_state.name;

  tree_widget.createVoidItem(
    path,LabelProperties{markerLabel(marker_state, scene_state, marker)}
  );

  ItemAdder adder{path, tree_widget};
  marker_paths.name = adder.addString("name:", name);
  {
    string label = "position: []";
    SceneState::XYZChannelsRef channels = marker_state.positionChannels();
    NumericProperties properties;

    marker_paths.position =
      TreePaths::Position(
        addXYZChannelsTo(adder, label, channels, properties)
      );
  }

  return marker_paths;
}


static TreePaths::Variable
createVariable(
  TreeWidget &tree_widget,
  const TreePath &path,
  const SceneState::Variable &variable_state
)
{
  tree_widget.createNumericItem(
    path,
    LabelProperties{variableLabel(variable_state)},
    variable_state.value,
    noMinimumNumericValue(),
    noMaximumNumericValue(),
    defaultDigitsOfPrecision()
  );

  ItemAdder adder{path, tree_widget};
  TreePath name_path = adder.addString("name:", variable_state.name);
  TreePaths::Variable variable_paths = {path, name_path};
  return variable_paths;
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


Optional<Marker> markerFromEnumerationValue(int enumeration_value)
{
  if (enumeration_value == 0) {
    return {};
  }
  else {
    return Marker(enumeration_value - 1);
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
pointName(
  const Optional<PointLink> &maybe_point,
  const SceneState::Markers &marker_states
)
{
  if (!maybe_point) {
    return "";
  }

  MarkerIndex marker_index = maybe_point->marker.index;
  return markerName(marker_index, marker_states);
}


static string
distanceErrorLabel(
  const SceneState::DistanceError &distance_error_state,
  const SceneState::Markers &marker_states
)
{
  string label = "[DistanceError]";

  string start_name =
    pointName(distance_error_state.optional_start, marker_states);

  string end_name =
    pointName(distance_error_state.optional_end, marker_states);

  if (start_name != "" && end_name != "") {
    label += " " + start_name + " <-> " + end_name;
  }
  else if (start_name != "") {
    label += " " + start_name;
  }
  else if (end_name != "") {
    label += " " + end_name;
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


static TreePath
createPoint(
  const string &label,
  const Optional<PointLink> &maybe_point,
  const TreeWidget::EnumerationOptions &marker_options,
  ItemAdder &adder
)
{
  Optional<MarkerIndex> optional_marker_index =
    makeMarkerIndexFromPoint(maybe_point);

  int enum_value = enumerationValueFromMarkerIndex(optional_marker_index);
  StringValue value = marker_options[enum_value];
  return adder.addString(label, value);
}


static TreePaths::DistanceError
createDistanceErrorInTree1(
  TreeWidget &tree_widget,
  const TreePath &path,
  const SceneState::Markers &marker_states,
  const SceneState::DistanceError &distance_error_state
)
{
  ItemAdder adder{path, tree_widget};
  string label = distanceErrorLabel(distance_error_state, marker_states);
  tree_widget.createVoidItem(path,LabelProperties{label});

  TreeWidget::EnumerationOptions marker_options =
    markerEnumerationOptions(marker_states);

  TreePath start_path =
    createPoint(
      "start:", distance_error_state.optional_start, marker_options, adder
    );

  TreePath end_path =
    createPoint(
      "end:", distance_error_state.optional_end, marker_options, adder
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


template <typename Visitor>
static void
visitPaths(Optional<TreePath> &maybe_v, const Visitor &visitor)
{
  if (maybe_v) {
    visitor(*maybe_v);
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
  TreePath mesh_path;
  TreePath body_path;
  TreePath marker_path;
  TreePath distance_error_path;
  TreePath variable_path;
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


static int
nVariablesOn(
  Optional<BodyIndex> maybe_body_index,
  const TreePaths &tree_paths,
  const SceneState &/*scene_state*/
)
{
  if (maybe_body_index) {
    return 0;
  }

  int n_variables = tree_paths.variables.size();
  int count = 0;

  for (int i=0; i!=n_variables; ++i) {
    if (!tree_paths.variables[i].path.empty()) {
      ++count;
    }
  }

  return count;
}


namespace {
struct BodyItemVisitor {
  virtual void visitName() const = 0;
  virtual void visitTranslation() const = 0;
  virtual void visitRotation() const = 0;
  virtual void visitScale() const = 0;
};
}


static void forEachBodyProperty(const BodyItemVisitor &visitor)
{
  visitor.visitName();
  visitor.visitTranslation();
  visitor.visitRotation();
  visitor.visitScale();
}


namespace {
struct BodyItemCounter : BodyItemVisitor {
  int &index;

  BodyItemCounter(int &index) : index(index) {}

  void visitName() const override { ++index; }
  void visitTranslation() const override { ++index; }
  void visitRotation() const override { ++index; }
  void visitScale() const override { ++index; }
};
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

  int n_variables = nVariablesOn({}, tree_paths, scene_state);
  const TreePath body_path = bodyPath(maybe_body_index, tree_paths);
  TreeItemIndex index = 0;

  if (maybe_body_index) {
    forEachBodyProperty(BodyItemCounter{index});
  }

  if (!maybe_body_index) {
    index += n_variables;
  }

  result.variable_path = childPath(body_path, index);

  int n_boxes = 0;
  int n_lines = 0;
  int n_meshes = 0;

  if (maybe_body_index) {
    n_boxes = tree_paths.body(*maybe_body_index).boxes.size();
    n_lines = tree_paths.body(*maybe_body_index).lines.size();
    n_meshes = tree_paths.body(*maybe_body_index).meshes.size();
  }
  else {
    // Can't add these to the scene
  }

  index += n_boxes;
  result.box_path = childPath(body_path, index);
  index += n_lines;
  result.line_path = childPath(body_path, index);
  index += n_meshes;
  result.mesh_path = childPath(body_path, index);
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
  Optional<BodyIndex> maybe_body_index,
  const TreePaths &tree_paths,
  const SceneState &scene_state
)
{
  return
    nextPaths(maybe_body_index, tree_paths, scene_state).distance_error_path;
}


void
  createDistanceErrorInTree(
    DistanceErrorIndex index,
    SceneTreeRef scene_tree,
    const SceneState &scene_state
  )
{
  TreeWidget &tree_widget = scene_tree.tree_widget;
  TreePaths &tree_paths = scene_tree.tree_paths;

  const SceneState::DistanceError &distance_error_state =
    scene_state.distance_errors[index];

  const TreePath next_distance_error_path =
    nextDistanceErrorPath(
      distance_error_state.maybe_body_index,
      tree_paths,
      scene_state
    );

  TreePath distance_error_path = next_distance_error_path;
  handlePathInsertion(tree_paths, distance_error_path);

  tree_paths.distance_errors.push_back(
    createDistanceErrorInTree1(
      tree_widget,
      distance_error_path,
      scene_state.markers(),
      distance_error_state
    )
  );
}


void
removeDistanceErrorFromTree(
  DistanceErrorIndex distance_error_index,
  SceneTreeRef scene_tree
)
{
  TreePaths &tree_paths = scene_tree.tree_paths;
  TreeWidget &tree_widget = scene_tree.tree_widget;
  TreePaths::DistanceErrors &distance_errors = tree_paths.distance_errors;
  TreePath distance_error_path = distance_errors[distance_error_index].path;
  tree_widget.removeItem(distance_error_path);
  removeIndexFrom(distance_errors, distance_error_index);
  handlePathRemoval(tree_paths, distance_error_path);
}


void
removeMarkerItemFromTree(
  MarkerIndex marker_index,
  SceneTreeRef scene_tree
)
{
  TreeWidget &tree_widget = scene_tree.tree_widget;
  TreePaths &tree_paths = scene_tree.tree_paths;
  TreePath marker_path = tree_paths.marker(marker_index).path;
  tree_widget.removeItem(marker_path);
  tree_paths.markers[marker_index].reset();
  handlePathRemoval(tree_paths, marker_path);
}


// It would be better to pass the path here so it's clear that this function
// can't remove the path from the tree_paths.
static void
removeVariableItemFromTree(
  const TreePaths::Variable &variable_paths,
  SceneTreeRef scene_tree
)
{
  TreeWidget &tree_widget = scene_tree.tree_widget;
  TreePaths &tree_paths = scene_tree.tree_paths;
  TreePath variable_path = variable_paths.path;
  tree_widget.removeItem(variable_path);
  handlePathRemoval(tree_paths, variable_path);
}


void
removeMarkerFromTree(
  MarkerIndex marker_index,
  SceneTreeRef scene_tree
)
{
  TreePaths &tree_paths = scene_tree.tree_paths;
  removeMarkerItemFromTree(marker_index, scene_tree);
  removeIndexFrom(tree_paths.markers, marker_index);
}


void
removeVariableFromTree(VariableIndex variable_index, SceneTreeRef scene_tree)
{
  TreePaths &tree_paths = scene_tree.tree_paths;

  const TreePaths::Variable &variable_paths =
    tree_paths.variables[variable_index];

  removeVariableItemFromTree(variable_paths, scene_tree);
  assert(variable_index < VariableIndex(tree_paths.variables.size()));
  removeIndexFrom(tree_paths.variables, variable_index);
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
  SceneTreeRef scene_tree,
  const SceneState &scene_state
)
{
  TreeWidget &tree_widget = scene_tree.tree_widget;
  TreePaths &tree_paths = scene_tree.tree_paths;
  const SceneState::Marker &state_marker = scene_state.marker(marker_index);
  Optional<BodyIndex> maybe_body_index = state_marker.maybe_body_index;

  const TreePath marker_path =
    nextMarkerPath(maybe_body_index, tree_paths, scene_state);

  handlePathInsertion(tree_paths, marker_path);

  tree_paths.markers[marker_index] =
    createMarker(
      tree_widget, marker_path, state_marker, scene_state, marker_index
    );
}


static void
createVariableItemInTree(
  VariableIndex variable_index,
  SceneTreeRef scene_tree,
  const SceneState &scene_state
)
{
  TreeWidget &tree_widget = scene_tree.tree_widget;
  TreePaths &tree_paths = scene_tree.tree_paths;

  const SceneState::Variable &variable_state =
    scene_state.variables[variable_index];

  const TreePath variable_path =
    nextPaths(/*maybe_body_index*/{}, tree_paths, scene_state).variable_path;

  handlePathInsertion(tree_paths, variable_path);

  tree_paths.variables[variable_index] =
    createVariable(tree_widget, variable_path, variable_state);
}


void
createMarkerInTree(
  MarkerIndex marker_index,
  SceneTreeRef scene_tree,
  const SceneState &scene_state
)
{
  TreeWidget &tree_widget = scene_tree.tree_widget;
  TreePaths &tree_paths = scene_tree.tree_paths;
  assert(marker_index == MarkerIndex(tree_paths.markers.size()));
  tree_paths.markers.emplace_back();

  createMarkerItemInTree(
    marker_index,
    {tree_widget, tree_paths},
    scene_state
  );
}


void
  createVariableInTree(
    VariableIndex variable_index,
    SceneTreeRef scene_tree,
    const SceneState &scene_state
  )
{
  TreeWidget &tree_widget = scene_tree.tree_widget;
  TreePaths &tree_paths = scene_tree.tree_paths;
  assert(variable_index == VariableIndex(tree_paths.variables.size()));
  tree_paths.variables.emplace_back();

  createVariableItemInTree(
    variable_index,
    {tree_widget, tree_paths},
    scene_state
  );
}


static TreePaths::XYZ
addXYZ(ItemAdder &adder, const string &label, const SceneState::XYZ &xyz)
{
  TreePath path = adder.addVoid(label);
  NumericProperties properties;
  return createXYZChildren(adder.tree_widget, path, xyz, properties);
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
    NumericProperties properties;
    properties.minimum_value = 0;
    SceneState::XYZChannelsRef channels = box_state.scaleChannels();
    const string label = "scale: []";

    TreePaths::XYZChannels xyz_paths =
      addXYZChannelsTo(adder, label, channels, properties);

    box_paths.scale = TreePaths::Scale(xyz_paths);
  }

  {
    NumericProperties properties;
    SceneState::XYZChannelsRef channels = box_state.centerChannels();
    const string label = "center: []";
    box_paths.center = addXYZChannelsTo(adder, label, channels, properties);
  }

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


static string str(int index)
{
  std::ostringstream stream;
  stream << index;
  return stream.str();
}


static string meshPositionLabel(MeshPositionIndex i, bool is_marked)
{
  if (!is_marked) {
    return str(i) + ": []";
  }
  else {
    return str(i) + ": []" + " (marked)";
  }
}


static TreePaths::Positions
addPositions(
  ItemAdder &adder,
  const string &label,
  const SceneState::MeshShape &mesh_shape
)
{
  TreePaths::Positions result;
  result.path = adder.addVoid(label);
  ItemAdder child_adder{result.path, adder.tree_widget};

  for (auto i : indicesOf(mesh_shape.positions)) {
    string label = meshPositionLabel(i, /*is_marked*/false);
    auto &position = mesh_shape.positions[i];
    TreePaths::XYZ xyz_paths = addXYZ(child_adder, label, position);
    emplaceInto(result.elements, i, xyz_paths);
  }

  return result;
}


static MeshPaths
createMeshItem(
  const TreePath &mesh_path,
  TreeWidget &tree_widget,
  const SceneState::Mesh &mesh_state
)
{
  MeshPaths mesh_paths;
  tree_widget.createVoidItem(mesh_path, LabelProperties("[Mesh]"));
  mesh_paths.path = mesh_path;
  ItemAdder adder{mesh_path, tree_widget};
  mesh_paths.scale = addXYZ(adder, "scale: []", mesh_state.scale);
  mesh_paths.center = addXYZ(adder, "center: []", mesh_state.center);
  mesh_paths.positions = addPositions(adder, "positions: []", mesh_state.shape);
  return mesh_paths;
}


namespace {
struct BodyItemCreator : BodyItemVisitor {
  BodyPaths &body_paths;
  ItemAdder &adder;
  const BodyState &body_state;

  BodyItemCreator(
    BodyPaths &body_paths,
    ItemAdder &adder,
    const BodyState &body_state
  )
  : body_paths(body_paths),
    adder(adder),
    body_state(body_state)
  {
  }

  void visitName() const override
  {
    body_paths.name = adder.addString("name:", body_state.name);
  }

  void visitTranslation() const override
  {
    TreeWidget &tree_widget = adder.tree_widget;
    TreePath translation_path = adder.addVoid("translation: []");

    body_paths.translation =
      TreePaths::Translation(
        createXYZChannels(
          tree_widget,
          translation_path,
          body_state.transform.translation,
          body_state.solve_flags.translation,
          body_state.expressions.translation,
          defaultDigitsOfPrecision()
        )
      );
  }

  void visitRotation() const override
  {
    TreeWidget &tree_widget = adder.tree_widget;
    TreePath rotation_path = adder.addVoid("rotation: []");

    body_paths.rotation =
      TreePaths::Rotation(
        createXYZChannels(
          tree_widget,
          rotation_path,
          body_state.transform.rotation,
          body_state.solve_flags.rotation,
          body_state.expressions.rotation,
          /*digits_of_precision*/1
        )
      );
  }

  void visitScale() const override
  {
    NumericProperties properties;
    properties.minimum_value = 0;

    AddChannelItemFunction add(adder);

    body_paths.scale =
      add(
        "scale:",
        body_state.transform.scale,
        body_state.solve_flags.scale,
        body_state.expressions.scale
      );
  }
};
}


namespace {
struct GeometryCreator {
  ItemAdder &adder;
  TreePaths::Body &body_paths;
  TreeWidget &tree_widget;
  const BodyState &body_state;

  void visitBox(BoxIndex i)
  {
    TreePath box_path = childPath(adder.parent_path, adder.n_children);

    emplaceInto(
      body_paths.boxes,
      i,
      createBoxItem(box_path, tree_widget, body_state.boxes[i])
    );

    ++adder.n_children;
  }

  void visitLine(LineIndex i)
  {
    TreePath line_path = childPath(adder.parent_path, adder.n_children);

    emplaceInto(
      body_paths.lines,
      i,
      createLineItem(line_path, tree_widget, body_state.lines[i])
    );

    ++adder.n_children;
  }

  void visitMesh(MeshIndex i)
  {
    TreePath mesh_path = childPath(adder.parent_path, adder.n_children);

    emplaceInto(
      body_paths.meshes,
      i,
      createMeshItem(
        mesh_path,
        tree_widget,
        body_state.meshes[i]
      )
    );

    ++adder.n_children;
  }
};
}


static TreePaths::Body
createBodyItem(
  const TreePath &body_path,
  const BodyState &body_state,
  TreeWidget &tree_widget
)
{
  TreePaths::Body body_paths;
  tree_widget.createVoidItem(body_path, LabelProperties{bodyLabel(body_state)});
  ItemAdder adder{body_path, tree_widget};

  body_paths.path = body_path;
  forEachBodyProperty(BodyItemCreator{body_paths, adder, body_state});

  GeometryCreator geometry_creator({
    adder,
    body_paths,
    tree_widget,
    body_state
  });

  forEachBodyGeometry(body_state, geometry_creator);

  return body_paths;
}


static void
createBodyItemInTree(
  BodyIndex body_index,
  SceneTreeRef scene_tree,
  const SceneState &scene_state
)
{
  TreeWidget &tree_widget = scene_tree.tree_widget;
  TreePaths &tree_paths = scene_tree.tree_paths;
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
  SceneTreeRef scene_tree,
  const SceneState &scene_state,
  BodyIndex body_index,
  BoxIndex box_index
)
{
  TreeWidget &tree_widget = scene_tree.tree_widget;
  TreePaths &tree_paths = scene_tree.tree_paths;
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
  SceneTreeRef scene_tree,
  const SceneState &scene_state,
  BodyIndex body_index,
  LineIndex line_index
)
{
  TreeWidget &tree_widget = scene_tree.tree_widget;
  TreePaths &tree_paths = scene_tree.tree_paths;
  TreePaths::Body &body_paths = tree_paths.body(body_index);
  const SceneState::Body &body_state = scene_state.body(body_index);
  TreePath line_path = nextPaths(body_index, tree_paths, scene_state).line_path;
  handlePathInsertion(tree_paths, line_path);

  assert(line_index == LineIndex(body_paths.lines.size()));
  body_paths.lines.emplace_back();

  body_paths.lines[line_index] =
    createLineItem(line_path, tree_widget, body_state.lines[line_index]);
}


void
createMeshInTree(
  SceneTreeRef scene_tree,
  const SceneState &scene_state,
  BodyIndex body_index,
  MeshIndex mesh_index
)
{
  const SceneState::Body &body_state = scene_state.body(body_index);
  TreeWidget &tree_widget = scene_tree.tree_widget;
  TreePaths &tree_paths = scene_tree.tree_paths;
  TreePaths::Body &body_paths = tree_paths.body(body_index);
  TreePath mesh_path = nextPaths(body_index, tree_paths, scene_state).mesh_path;
  handlePathInsertion(tree_paths, mesh_path);
  assert(mesh_index == MeshIndex(body_paths.meshes.size()));
  body_paths.meshes.emplace_back();

  body_paths.meshes[mesh_index] =
    createMeshItem(
      mesh_path,
      tree_widget,
      body_state.meshes[mesh_index]
    );
}


// This function creates the body item and all child items and stores
// the path in the tree_paths at the proper index.  It does not shift
// any indices.
void
createBodyInTree(
  BodyIndex body_index,
  SceneTreeRef scene_tree,
  const SceneState &scene_state
)
{
  TreePaths &tree_paths = scene_tree.tree_paths;

  if (body_index >= BodyIndex(tree_paths.bodies.size())) {
    tree_paths.bodies.resize(body_index+1);
  }

  assert(!tree_paths.bodies[body_index]);
  createBodyItemInTree(body_index, scene_tree, scene_state);

  for (auto i : indicesOfMarkersOnBody(body_index, scene_state)) {
    createMarkerInTree(i, scene_tree, scene_state);
  }

  for (auto i : indicesOfDistanceErrorsOnBody(body_index, scene_state)) {
    createDistanceErrorInTree(i, scene_tree, scene_state);
  }

  for (auto i : indicesOfChildBodies(body_index, scene_state)) {
    createBodyInTree(i, scene_tree, scene_state);
  }
}


static void
removeBodyItemFromTree(BodyIndex body_index, SceneTreeRef scene_tree)
{
  TreeWidget &tree_widget = scene_tree.tree_widget;
  TreePaths &tree_paths = scene_tree.tree_paths;
  TreePath body_path = tree_paths.body(body_index).path;
  tree_widget.removeItem(body_path);
  tree_paths.bodies[body_index].reset();
  handlePathRemoval(tree_paths, body_path);
}


void
removeBodyFromTree(
  SceneTreeRef scene_tree,
  const SceneState &
#ifndef NDEBUG
    scene_state
#endif
    ,
  BodyIndex body_index
)
{
  TreeWidget &tree_widget = scene_tree.tree_widget;
  TreePaths &tree_paths = scene_tree.tree_paths;
  assert(indicesOfMarkersOnBody(body_index, scene_state).empty());
  assert(indicesOfChildBodies(body_index, scene_state).empty());
  removeBodyItemFromTree(body_index, {tree_widget, tree_paths});
  removeIndexFrom(tree_paths.bodies, body_index);
}


void
removeBoxFromTree(
  SceneTreeRef scene_tree,
  const SceneState &,
  BodyIndex body_index,
  BoxIndex box_index
)
{
  TreeWidget &tree_widget = scene_tree.tree_widget;
  TreePaths &tree_paths = scene_tree.tree_paths;
  TreePath box_path = tree_paths.body(body_index).boxes[box_index].path;
  tree_widget.removeItem(box_path);
  removeIndexFrom(tree_paths.bodies[body_index]->boxes, box_index);
  handlePathRemoval(tree_paths, box_path);
}


void
removeLineFromTree(
  SceneTreeRef scene_tree,
  const SceneState &,
  BodyIndex body_index,
  LineIndex line_index
)
{
  TreeWidget &tree_widget = scene_tree.tree_widget;
  TreePaths &tree_paths = scene_tree.tree_paths;
  TreePath line_path = tree_paths.body(body_index).lines[line_index].path;
  tree_widget.removeItem(line_path);
  removeIndexFrom(tree_paths.bodies[body_index]->lines, line_index);
  handlePathRemoval(tree_paths, line_path);
}


void
removeMeshFromTree(
  SceneTreeRef scene_tree,
  const SceneState &,
  BodyIndex body_index,
  MeshIndex mesh_index
)
{
  // This is copied from removeLineFromTree()
  TreeWidget &tree_widget = scene_tree.tree_widget;
  TreePaths &tree_paths = scene_tree.tree_paths;
  TreePath mesh_path = tree_paths.body(body_index).meshes[mesh_index].path;
  tree_widget.removeItem(mesh_path);
  removeIndexFrom(tree_paths.bodies[body_index]->meshes, mesh_index);
  handlePathRemoval(tree_paths, mesh_path);
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

  SceneTreeRef scene_tree = {tree_widget, tree_paths};

  for (auto i : indicesOfChildBodies({}, scene_state)) {
    createBodyInTree(i, scene_tree, scene_state);
  }

  for (auto i : indicesOfMarkersOnBody({}, scene_state)) {
    createMarkerInTree(i, scene_tree, scene_state);
  }

  for (auto i : indicesOf(scene_state.variables)) {
    createVariableInTree(i, scene_tree, scene_state);
  }

  for (auto i : indicesOfDistanceErrorsOnBody({}, scene_state)) {
    createDistanceErrorInTree(i, scene_tree, scene_state);
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
updateNumericValue(
  TreeWidget &tree_widget, const TreePath &path, NumericValue value
)
{
  tree_widget.setItemNumericValue(path, value);
}


static void
updateNumericValue(
  TreeWidget &tree_widget,
  const ChannelPaths &channel,
  NumericValue value
)
{
  updateNumericValue(tree_widget, channel.path, value);
}


template <typename XYZPaths>
static void
  updateXYZValues(
    TreeWidget &tree_widget,
    const XYZPaths &paths,
    const Vec3 &value
  )
{
  updateNumericValue(tree_widget, paths.x, value.x);
  updateNumericValue(tree_widget, paths.y, value.y);
  updateNumericValue(tree_widget, paths.z, value.z);
}


void
updateTreeMarkerItem(
  Marker marker,
  TreeWidget &tree_widget,
  const TreePaths &tree_paths,
  const SceneState &scene_state
)
{
  const SceneState::Markers &markers = scene_state.markers();

  tree_widget.setItemLabel(
    tree_paths.marker(marker.index).path,
    markerLabel(markers[marker.index], scene_state, marker)
  );
}


static void
updateMarker(
  MarkerIndex i,
  TreeWidget &tree_widget,
  const TreePaths &tree_paths,
  const SceneState &scene_state
)
{
  const SceneState::Markers &markers = scene_state.markers();
  updateTreeMarkerItem(Marker(i), tree_widget, tree_paths, scene_state);

  updateXYZValues(
    tree_widget,
    tree_paths.marker(i).position,
    vec3FromXYZState(markers[i].position)
  );
}


static void
updateVariable(
  VariableIndex i,
  TreeWidget &tree_widget,
  const TreePaths &tree_paths,
  const SceneState &scene_state
)
{
  tree_widget.setItemLabel(
    tree_paths.variables[i].path,
    variableLabel(scene_state.variables[i])
  );
}


namespace {
struct BodyPropertyUpdator {
  const TreePaths::Body &body_paths;
  const SceneState::Body &body_state;
  TreeWidget &tree_widget;

  void visitName()
  {
    tree_widget.setItemLabel(body_paths.path, bodyLabel(body_state));
  }

  void visitTranslation()
  {
    const TransformState &transform_state = body_state.transform;
    const TreePaths::Translation &translation_paths = body_paths.translation;
    const TranslationState &translation = translationStateOf(transform_state);

    updateXYZValues(
      tree_widget, translation_paths, vec3FromXYZState(translation)
    );
  }

  void visitRotation()
  {
    const TransformState &transform_state = body_state.transform;
    const TreePaths::Rotation &rotation_paths = body_paths.rotation;
    const RotationState &rotation = rotationStateOf(transform_state);
    updateXYZValues(tree_widget, rotation_paths, rotationValuesDeg(rotation));
  }

  void visitScale()
  {
    updateNumericValue(
      tree_widget, body_paths.scale, body_state.transform.scale
    );
  }
};
}


namespace {
struct TreeGeometryUpdater {
  const TreePaths::Body &body_paths;
  const SceneState::Body &body_state;
  TreeWidget &tree_widget;

  void visitBox(BoxIndex i)
  {
    const BoxPaths &box_paths = body_paths.boxes[i];
    const SceneState::Box &box_state = body_state.boxes[i];
    {
      const TreePaths::XYZChannels &scale_paths = box_paths.scale;
      const SceneState::XYZ &scale = box_state.scale;
      updateXYZValues(tree_widget, scale_paths, vec3FromXYZState(scale));
    }
    {
      const TreePaths::XYZChannels &center_paths = box_paths.center;
      const SceneState::XYZ &center = box_state.center;
      updateXYZValues(tree_widget, center_paths, vec3FromXYZState(center));
    }
  }

  void visitLine(LineIndex)
  {
  }

  void visitMesh(MeshIndex i)
  {
    const MeshPaths &mesh_paths = body_paths.meshes[i];
    const SceneState::Mesh &mesh_state = body_state.meshes[i];
    {
      const TreePaths::XYZ &scale_paths = mesh_paths.scale;
      const SceneState::XYZ &scale = mesh_state.scale;
      updateXYZValues(tree_widget, scale_paths, vec3FromXYZState(scale));
    }
    {
      const TreePaths::XYZ &center_paths = mesh_paths.center;
      const SceneState::XYZ &center = mesh_state.center;
      updateXYZValues(tree_widget, center_paths, vec3FromXYZState(center));
    }
  }
};
}


void
updateTreeBodyMeshPosition(
  TreeWidget &tree_widget,
  const TreePaths &tree_paths,
  const SceneState &scene_state,
  BodyMeshPosition body_mesh_position
)
{
  BodyMesh body_mesh = body_mesh_position.array.body_mesh;
  Body body = body_mesh.body;
  BodyIndex body_index = body.index;
  MeshIndex mesh_index = body_mesh.index;
  MeshPositionIndex position_index = body_mesh_position.index;

  bool is_marked =
    (scene_state.maybe_marked_body_mesh_position == body_mesh_position);

  const TreePaths::XYZ &mesh_position_paths =
    tree_paths
    .body(body_index)
    .meshes[mesh_index]
    .positions
    .elements[position_index];

  tree_widget.setItemLabel(
    mesh_position_paths.path,
    meshPositionLabel(position_index, is_marked)
  );

  const SceneState::XYZ &mesh_position_state =
    scene_state
    .body(body_index)
    .meshes[mesh_index]
    .shape.positions[position_index];

  Vec3 position = vec3FromXYZState(mesh_position_state);
  updateXYZValues(tree_widget, mesh_position_paths, position);
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
  const TreePaths::Body &body_paths = tree_paths.body(body_index);

  BodyPropertyUpdator
    body_property_updator{body_paths, body_state, tree_widget};

  body_property_updator.visitName();
  body_property_updator.visitTranslation();
  body_property_updator.visitRotation();
  body_property_updator.visitScale();

  assert(body_paths.boxes.size() == body_state.boxes.size());

  TreeGeometryUpdater tree_geometry_updater({
    body_paths,
    body_state,
    tree_widget
  });

  forEachBodyGeometry(body_state, tree_geometry_updater);
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
    updateMarker(i, tree_widget, tree_paths, state);
  }

  for (auto i : indicesOf(tree_paths.distance_errors)) {
    updateDistanceError(i, tree_widget, tree_paths, state);
  }

  for (auto i : indicesOf(tree_paths.variables)) {
    updateVariable(i, tree_widget, tree_paths, state);
  }

  tree_widget.setItemLabel(
    tree_paths.total_error, totalErrorLabel(state.total_error)
  );
}


static void
updateTreeDistanceError(
  DistanceError distance_error,
  TreeWidget &tree_widget,
  const TreePaths &tree_paths,
  const SceneState &scene_state
)
{
  const SceneState::Markers &state_markers = scene_state.markers();

  TreeWidget::EnumerationOptions marker_options =
    markerEnumerationOptions(state_markers);

  DistanceErrorIndex i = distance_error.index;

  const TreePaths::DistanceError &distance_error_paths =
    tree_paths.distance_errors[i];

  const SceneState::DistanceError &distance_error_state =
    scene_state.distance_errors[i];

  const TreePath &start_path = distance_error_paths.start;
  const TreePath &end_path = distance_error_paths.end;

  Optional<MarkerIndex> optional_start_marker_index =
    makeMarkerIndexFromPoint(distance_error_state.optional_start);

  Optional<MarkerIndex> optional_end_marker_index =
    makeMarkerIndexFromPoint(distance_error_state.optional_end);

  int start_value =
    enumerationValueFromMarkerIndex(optional_start_marker_index);

  int end_value =
    enumerationValueFromMarkerIndex(optional_end_marker_index);

  tree_widget.setItemStringValue(
    start_path, marker_options[start_value]
  );

  tree_widget.setItemStringValue(
    end_path, marker_options[end_value]
  );

  tree_widget.setItemLabel(
    distance_error_paths.path,
    distanceErrorLabel(distance_error_state, scene_state.markers())
  );
}


void
updateTreeDistanceErrorMarkerOptions(
  TreeWidget &tree_widget,
  const TreePaths &tree_paths,
  const SceneState &scene_state
)
{
  for (auto i : indicesOf(tree_paths.distance_errors)) {
    updateTreeDistanceError(
      DistanceError(i), tree_widget, tree_paths, scene_state
    );
  }
}


void
setTreeBoolValue(
  TreeWidget &tree_widget,
  const TreePath &path,
  bool new_state
)
{
  tree_widget.setItemBoolValue(path, new_state);
}


static bool isMatchingPath(const TreePath &path1, const TreePath &path2)
{
  return path1 == path2;
}


static bool
isMatchingPath(const TreePath &path, const ChannelPaths &channel)
{
  return startsWith(path, channel.path);
}


template <typename Component>
static XYZComponent
matchingComponent(
  const TreePath &path,
  const TreePaths::BasicXYZ<Component> &xyz_path
)
{
  if (isMatchingPath(path, xyz_path.x)) return XYZComponent::x;
  if (isMatchingPath(path, xyz_path.y)) return XYZComponent::y;
  if (isMatchingPath(path, xyz_path.z)) return XYZComponent::z;
  assert(false); // not implemented
  return XYZComponent::x;
}


static NumericValue &numericValue(SceneState::XYZ &v, XYZComponent component)
{
  switch (component) {
    case XYZComponent::x: return v.x;
    case XYZComponent::y: return v.y;
    case XYZComponent::z: return v.z;
  }

  assert(false); // shouldn't happen
  return v.x;
}


static NumericValue &
numericValue(BodyTranslationComponent component, SceneState &scene_state)
{
  Body body = bodyOf(component);
  SceneState::XYZ &v = scene_state.body(body.index).transform.translation;
  return numericValue(v, component.component);
}


static NumericValue &
numericValue(BodyRotationComponent component, SceneState &scene_state)
{
  Body body = bodyOf(component);
  SceneState::XYZ &v = scene_state.body(body.index).transform.rotation;
  return numericValue(v, component.component);
}


static NumericValue &
numericValue(BodyScale element, SceneState &scene_state)
{
  return scene_state.body(element.body.index).transform.scale;
}


static NumericValue &
numericValue(BodyBoxScaleComponent element, SceneState &scene_state)
{
  SceneState::Body &body_state = scene_state.body(bodyOf(element).index);
  SceneState::Box &box_state = body_state.boxes[bodyBoxOf(element).index];
  return numericValue(box_state.scale, element.component);
}


static NumericValue &
numericValue(BodyBoxCenterComponent element, SceneState &scene_state)
{
  const BodyBox &body_box = bodyBoxOf(element);
  SceneState::Body &body_state = scene_state.body(body_box.body.index);
  SceneState::Box &box_state = body_state.boxes[body_box.index];
  return numericValue(box_state.center, element.component);
}


static NumericValue &
numericValue(BodyLineStartComponent element, SceneState &scene_state)
{
  LineIndex line_index = bodyLineOf(element).index;
  SceneState::Body &body_state = scene_state.body(bodyOf(element).index);
  SceneState::Line &line_state = body_state.lines[line_index];
  return numericValue(line_state.start, element.component);
}


static NumericValue &
numericValue(BodyLineEndComponent element, SceneState &scene_state)
{
  LineIndex line_index = bodyLineOf(element).index;
  SceneState::Body &body_state = scene_state.body(bodyOf(element).index);
  SceneState::Line &line_state = body_state.lines[line_index];
  return numericValue(line_state.end, element.component);
}


static SceneState::Mesh &meshState(BodyMesh body_mesh, SceneState &scene_state)
{
  BodyIndex body_index = body_mesh.body.index;
  SceneState::Body &body_state = scene_state.body(body_index);
  SceneState::Mesh &mesh_state = body_state.meshes[body_mesh.index];
  return mesh_state;
}


static NumericValue &
numericValue(BodyMeshScaleComponent element, SceneState &scene_state)
{
  SceneState::Mesh &mesh_state = meshState(bodyMeshOf(element), scene_state);
  SceneState::XYZ &xyz_state = mesh_state.scale;
  return numericValue(xyz_state, element.component);
}


static NumericValue &
numericValue(BodyMeshCenterComponent element, SceneState &scene_state)
{
  SceneState::Mesh &mesh_state = meshState(bodyMeshOf(element), scene_state);
  SceneState::XYZ &xyz_state = mesh_state.center;
  return numericValue(xyz_state, element.component);
}


static NumericValue &
numericValue(DistanceErrorDesiredDistance element, SceneState &scene_state)
{
  return
    scene_state.distance_errors[element.distance_error.index].desired_distance;
}


static NumericValue &
numericValue(DistanceErrorWeight element, SceneState &scene_state)
{
  return scene_state.distance_errors[element.distance_error.index].weight;
}


static NumericValue &
numericValue(VariableValue element, SceneState &scene_state)
{
  SceneState::Variable &variable_state =
    scene_state.variables[element.variable.index];

  return variable_state.value;
}


static NumericValue &
numericValue(MarkerPositionComponent element, SceneState &scene_state)
{
  MarkerIndex marker_index = markerOf(element).index;
  SceneState::Marker &marker_state = scene_state.marker(marker_index);
  SceneState::XYZ &xyz_state = marker_state.position;
  return numericValue(xyz_state, element.component);
}


static NumericValue &
numericValue(BodyMeshPositionComponent element, SceneState &scene_state)
{
  BodyMesh body_mesh = bodyMeshOf(element);
  BodyIndex body_index = body_mesh.body.index;
  SceneState::Body &body_state = scene_state.body(body_index);
  MeshIndex mesh_index = body_mesh.index;

  auto &position_state =
    body_state.meshes[mesh_index].shape.positions[element.parent.index];

  SceneState::XYZ &xyz_state = position_state;
  return numericValue(xyz_state, element.component);
}


namespace {
struct SceneElementVisitor {
  virtual void visitMarkerPositionComponent(Marker, XYZComponent) { }
  virtual void visitMarkerName(Marker) { }
  virtual void visitBodyScale(Body) { }
  virtual void visitBodyName(Body) { }
  virtual void visitBodyTranslationComponent(Body, XYZComponent) { }
  virtual void visitBodyRotationComponent(Body, XYZComponent) { }
  virtual void visitBodyBoxScaleComponent(BodyBox, XYZComponent) { }
  virtual void visitBodyBoxCenterComponent(BodyBox, XYZComponent) { }
  virtual void visitBodyLineStartComponent(BodyLine, XYZComponent) { }
  virtual void visitBodyLineEndComponent(BodyLine, XYZComponent) { }
  virtual void visitBodyMeshScaleComponent(BodyMesh, XYZComponent) { }
  virtual void visitBodyMeshCenterComponent(BodyMesh, XYZComponent) { }

  virtual void visitBodyMeshPositionComponent(BodyMeshPosition, XYZComponent)
  {
  }

  virtual void visitDistanceErrorDesiredDistance(DistanceError) { }
  virtual void visitDistanceErrorWeight(DistanceError) { }
  virtual void visitVariableValue(Variable) { }
  virtual void visitVariableName(Variable) { }
};
}


namespace {
struct SetStringValueVisitor : SceneElementVisitor {
  SceneState &scene_state;
  const StringValue &value;
  bool &is_name;

  SetStringValueVisitor(
    SceneState &scene_state,
    const StringValue &value,
    bool &is_name
  )
  : SceneElementVisitor(),
    scene_state(scene_state),
    value(value),
    is_name(is_name)
  {
  }

  void visitBodyName(Body body) override
  {
    is_name = true;

    if (findBodyWithName(scene_state, value)) {
      // Can't allow duplicate body names.
      return;
    }

    BodyIndex body_index = body.index;
    SceneState::Body &body_state = scene_state.body(body_index);
    body_state.name = value;
  }

  void visitMarkerName(Marker marker) override
  {
    is_name = true;

    if (findMarkerWithName(scene_state, value)) {
      // Can't allow duplicate marker names.
      return;
    }

    MarkerIndex marker_index = marker.index;
    SceneState::Marker &marker_state = scene_state.marker(marker_index);
    marker_state.name = value;
  }

  void visitVariableName(Variable variable) override
  {
    is_name = true;

    if (findVariableWithName(scene_state, value)) {
      // Can't allow duplicate variable names.
      return;
    }

    SceneState::Variable &variable_state =
      scene_state.variables[variable.index];

    variable_state.name = value;
    return;
  }
};
}


namespace {
template <typename Function>
struct NumericScenePathVisitor : SceneElementVisitor {
  const Function &f;

  NumericScenePathVisitor(const Function &f)
  : SceneElementVisitor(),
    f(f)
  {
  }

  template <typename Element>
  void visitElement(Element element)
  {
    f(element);
  }

  void visitBodyTranslationComponent(Body body, XYZComponent component) override
  {
    visitElement(BodyTranslationComponent{{body}, component});
  }

  void visitBodyRotationComponent(Body body, XYZComponent component) override
  {
    visitElement(BodyRotationComponent{{body}, component});
  }

  void visitBodyScale(Body body) override
  {
    visitElement(BodyScale{body});
  }

  void
  visitBodyBoxScaleComponent(BodyBox body_box, XYZComponent component) override
  {
    visitElement(BodyBoxScaleComponent{{body_box}, component});
  }

  void
  visitBodyBoxCenterComponent(BodyBox body_box, XYZComponent component) override
  {
    visitElement(BodyBoxCenterComponent{{body_box}, component});
  }

  void
  visitBodyLineStartComponent(
    BodyLine body_line, XYZComponent component
  ) override
  {
    visitElement(BodyLineStartComponent{{body_line}, component});
  }

  void
  visitBodyLineEndComponent(
    BodyLine body_line, XYZComponent component
  ) override
  {
    visitElement(BodyLineEndComponent{{body_line}, component});
  }

  void
  visitBodyMeshPositionComponent(
    BodyMeshPosition body_mesh_position, XYZComponent component
  ) override
  {
    visitElement(BodyMeshPositionComponent{body_mesh_position, component});
  }

  void
  visitMarkerPositionComponent(Marker marker, XYZComponent component) override
  {
    visitElement(MarkerPositionComponent{{marker}, component});
  }

  void
  visitBodyMeshScaleComponent(
    BodyMesh body_mesh, XYZComponent component
  ) override
  {
    visitElement(BodyMeshScaleComponent{{body_mesh}, component});
  }

  void
  visitBodyMeshCenterComponent(
    BodyMesh body_mesh, XYZComponent component
  ) override
  {
    visitElement(BodyMeshCenterComponent{{body_mesh}, component});
  }

  void
  visitDistanceErrorDesiredDistance(
    DistanceError distance_error
  ) override
  {
    visitElement(DistanceErrorDesiredDistance{distance_error});
  }

  void
  visitDistanceErrorWeight(DistanceError distance_error) override
  {
    visitElement(DistanceErrorWeight{distance_error});
  }

  void visitVariableValue(Variable variable) override
  {
    visitElement(VariableValue{variable});
  }
};
}


static BodyRotationComponent
bodyRotationComponent(BodyIndex body_index, XYZComponent component)
{
  return
    BodyRotationComponent{
      BodyRotation{Body(body_index)},
      component
    };
}


namespace {
struct PathMatcher {
  const TreePath &path;
  SceneElementVisitor &visitor;

  void
  visitMarkerPosition(
    Marker marker, const TreePaths::Position &position_paths
  )
  {
    XYZComponent component = matchingComponent(path, position_paths);
    visitor.visitMarkerPositionComponent(marker, component);
    return;
  }

  void visitMarker(Marker marker, const MarkerPaths &marker_paths)
  {
    if (startsWith(path, marker_paths.position.path)) {
      visitMarkerPosition(marker, marker_paths.position);
      return;
    }

    if (startsWith(path, marker_paths.name)) {
      visitor.visitMarkerName(marker);
      return;
    }
  }

  void visitBodyLineStart(BodyLine body_line, const TreePaths::XYZ &xyz_paths)
  {
    XYZComponent component = matchingComponent(path, xyz_paths);
    visitor.visitBodyLineStartComponent(body_line, component);
  }

  void visitBodyLineEnd(BodyLine body_line, const TreePaths::XYZ &xyz_paths)
  {
    XYZComponent component = matchingComponent(path, xyz_paths);
    visitor.visitBodyLineEndComponent(body_line, component);
  }

  void visitBodyLine(BodyLine body_line, const LinePaths &line_paths)
  {
    if (startsWith(path, line_paths.start.path)) {
      visitBodyLineStart(body_line, line_paths.start);
      return;
    }

    if (startsWith(path, line_paths.end.path)) {
      visitBodyLineEnd(body_line, line_paths.end);
      return;
    }

    assert(false); // shouldn't happen
  }

  void
  visitBodyBoxScale(BodyBox body_box, const TreePaths::Scale &box_scale_paths)
  {
    XYZComponent component = matchingComponent(path, box_scale_paths);
    visitor.visitBodyBoxScaleComponent(body_box, component);
  }

  void
  visitBodyBoxCenter(
    BodyBox body_box,
    const TreePaths::XYZChannels &body_box_center_paths
  )
  {
    XYZComponent component = matchingComponent(path, body_box_center_paths);
    visitor.visitBodyBoxCenterComponent(body_box, component);
  }

  void visitBodyBox(BodyBox body_box, const BoxPaths &box_paths)
  {
    if (startsWith(path, box_paths.scale.path)) {
      visitBodyBoxScale(body_box, box_paths.scale);
      return;
    }

    if (startsWith(path, box_paths.center.path)) {
      visitBodyBoxCenter(body_box, box_paths.center);
      return;
    }

    assert(false); // not implemented
  }

  void
  visitBodyTranslation(
    Body body, const TreePaths::Translation &translation_paths
  )
  {
    XYZComponent component = matchingComponent(path, translation_paths);
    visitor.visitBodyTranslationComponent(body, component);
  }

  void visitBodyRotation(Body body, const TreePaths::Rotation &rotation_paths)
  {
    XYZComponent component = matchingComponent(path, rotation_paths);
    visitor.visitBodyRotationComponent(body, component);
  }

  bool visitBodyBoxes(Body body, const BodyPaths &body_paths)
  {
    for (BoxIndex box_index : indicesOf(body_paths.boxes)) {
      const BoxPaths &box_paths = body_paths.boxes[box_index];

      if (startsWith(path, box_paths.path)) {
        visitBodyBox({body, box_index}, box_paths);
        return true;
      }
    }

    return false;
  }

  bool visitBodyLines(Body body, const BodyPaths &body_paths)
  {
    for (LineIndex line_index : indicesOf(body_paths.lines)) {
      const LinePaths &line_paths = body_paths.lines[line_index];

      if (startsWith(path, line_paths.path)) {
        visitBodyLine({body, line_index}, line_paths);
        return true;
      }
    }

    return false;
  }

  bool visitBodyMeshes(Body body, const BodyPaths &body_paths)
  {
    for (MeshIndex mesh_index : indicesOf(body_paths.meshes)) {
      const MeshPaths &mesh_paths = body_paths.meshes[mesh_index];

      if (startsWith(path, mesh_paths.path)) {
        visitBodyMesh(body.mesh(mesh_index), mesh_paths);
        return true;
      }
    }

    return false;
  }

  bool visitBody(Body body, const BodyPaths &body_paths)
  {
    if (path == body_paths.name) {
      visitor.visitBodyName(body);
      return true;
    }

    if (startsWith(path, body_paths.translation.path)) {
      visitBodyTranslation(body, body_paths.translation);
      return true;
    }

    if (startsWith(path, body_paths.rotation.path)) {
      visitBodyRotation(body, body_paths.rotation);
      return true;
    }

    if (isMatchingPath(path, body_paths.scale)) {
      visitor.visitBodyScale(body);
      return true;
    }

    struct GeometryTypeVisitor {
      PathMatcher &path_matcher;
      Body body;
      const BodyPaths &body_paths;
      bool &matched;

      void visitBoxes() const
      {
        if (!matched) {
          if (path_matcher.visitBodyBoxes(body, body_paths)) {
            matched = true;
          }
        }
      }

      void visitLines() const
      {
        if (!matched) {
          if (path_matcher.visitBodyLines(body, body_paths)) {
            matched = true;
          }
        }
      }

      void visitMeshes() const
      {
        if (!matched) {
          if (path_matcher.visitBodyMeshes(body, body_paths)) {
            matched = true;
          }
        }
      }
    };

    bool matched = false;
    GeometryTypeVisitor geometry_type_visitor{*this, body, body_paths, matched};
    forEachBodyGeometryType(geometry_type_visitor);
    return matched;
  }

  void
  visitDistanceError(
    DistanceError distance_error,
    const TreePaths::DistanceError &distance_error_paths
  )
  {
    if (startsWith(path, distance_error_paths.desired_distance)) {
      visitor.visitDistanceErrorDesiredDistance(distance_error);
      return;
    }

    if (startsWith(path, distance_error_paths.weight)) {
      visitor.visitDistanceErrorWeight(distance_error);
      return;
    }
  }

  void
  visitVariable(
    Variable variable,
    const TreePaths::Variable &variable_paths
  )
  {
    if (path == variable_paths.path) {
      visitor.visitVariableValue(variable);
      return;
    }

    if (startsWith(path, variable_paths.name)) {
      visitor.visitVariableName(variable);
      return;
    }
  }

  void
  visitBodyMeshPosition(
    BodyMeshPosition element, const TreePaths::XYZ &position_paths
  )
  {
    const TreePaths::XYZ &xyz_path = position_paths;
    XYZComponent component = matchingComponent(path, xyz_path);
    visitor.visitBodyMeshPositionComponent(element, component);
    return;
  }

  void
  visitBodyMeshPositions(
    BodyMesh body_mesh, const TreePaths::Positions &mesh_positions_paths
  )
  {
    for (MeshPositionIndex i : indicesOf(mesh_positions_paths.elements)) {
      const TreePaths::XYZ &position_paths = mesh_positions_paths.elements[i];

      if (startsWith(path, position_paths.path)) {
        BodyMeshPosition element{body_mesh.positions(), i};
        visitBodyMeshPosition(element, position_paths);
        return;
      }
    }

    assert(false); // shouldn't happen
  }

  void visitBodyMesh(BodyMesh body_mesh, const MeshPaths &mesh_paths)
  {
    if (startsWith(path, mesh_paths.scale.path)) {
      const TreePaths::XYZ &xyz_path = mesh_paths.scale;
      XYZComponent component = matchingComponent(path, xyz_path);
      visitor.visitBodyMeshScaleComponent(body_mesh, component);
      return;
    }

    if (startsWith(path, mesh_paths.center.path)) {
      const TreePaths::XYZ &xyz_path = mesh_paths.center;
      XYZComponent component = matchingComponent(path, xyz_path);
      visitor.visitBodyMeshCenterComponent(body_mesh, component);
      return;
    }

    if (startsWith(path, mesh_paths.positions.path)) {
      visitBodyMeshPositions(body_mesh, mesh_paths.positions);
      return;
    }

    assert(false); // shouldn't happen
  }

  void visitScene(const TreePaths &tree_paths)
  {
    for (auto i : indicesOf(tree_paths.markers)) {
      const MarkerPaths &marker_paths = tree_paths.marker(i);

      if (startsWith(path, tree_paths.marker(i).path)) {
        visitMarker(i, marker_paths);
        return;
      }
    }

    for (auto i : indicesOf(tree_paths.distance_errors)) {
      auto &distance_error_paths = tree_paths.distance_errors[i];

      if (startsWith(path, distance_error_paths.path)) {
        visitDistanceError(i, distance_error_paths);
        return;
      }
    }

    for (auto i : indicesOf(tree_paths.variables)) {
      const TreePaths::Variable &variable_paths = tree_paths.variables[i];

      if (startsWith(path, variable_paths.path)) {
        visitVariable(i, variable_paths);
        return;
      }
    }

    for (auto i : indicesOf(tree_paths.bodies)) {
      const BodyPaths &body_paths = tree_paths.body(i);

      if (startsWith(path, body_paths.path)) {
        if (visitBody(i, body_paths)) {
          return;
        }
        else {
          // There might be a child body that matches.
        }
      }
    }
  }
};
}


static void
forMatchingScenePath(
  const TreePath &path,
  SceneElementVisitor &visitor,
  const TreePaths &tree_paths
)
{
  PathMatcher{path, visitor}.visitScene(tree_paths);
}


template <typename Function>
static void
forMatchingNumericScenePath(
  const TreePath &path, const TreePaths &tree_paths, const Function &f
)
{
  NumericScenePathVisitor<Function> visitor = { f };
  forMatchingScenePath(path, visitor, tree_paths);
}


bool
setSceneStateNumericValue(
  SceneState &scene_state,
  const TreePath &path,
  NumericValue value,
  const TreePaths &tree_paths
)
{
  bool was_changed = false;

  auto set_value_function = [&](auto element){
    numericValue(element, scene_state) = value;
    was_changed = true;
  };

  forMatchingNumericScenePath(path, tree_paths, set_value_function);
  return was_changed;
}


bool
setSceneStateStringValue(
  SceneState &scene_state,
  const TreePath &path,
  const StringValue &value,
  const TreePaths &tree_paths
)
{
  bool is_name = false;
  SetStringValueVisitor visitor{scene_state, value, is_name };
  forMatchingScenePath(path, visitor, tree_paths);
  return is_name;
}


void
removeBodyBranchItemsFromTree(
  BodyIndex body_index,
  SceneTreeRef scene_tree,
  const SceneState &scene_state
)
{
  struct Visitor {
    SceneTreeRef scene_tree;

    void visitBody(BodyIndex body_index) const
    {
      removeBodyItemFromTree(body_index, scene_tree);
    }

    void visitMarker(MarkerIndex marker_index) const
    {
      removeMarkerItemFromTree(marker_index, scene_tree);
    }
  } visitor = {scene_tree};

  forEachBranchIndexInPostOrder(body_index, scene_state, visitor);
}


void
createBodyBranchItemsInTree(
  BodyIndex body_index,
  SceneTreeRef scene_tree,
  const SceneState &scene_state
)
{
  struct Visitor {
    SceneTreeRef scene_tree;
    const SceneState &scene_state;

    void visitBody(BodyIndex body_index) const
    {
      createBodyItemInTree(body_index, scene_tree, scene_state);
    }

    void visitMarker(MarkerIndex marker_index) const
    {
      createMarkerItemInTree(marker_index, scene_tree, scene_state);
    }
  } visitor = {scene_tree, scene_state};

  forEachBranchIndexInPreOrder(body_index, scene_state, visitor);
}



namespace {
struct SolvableScenePathVisitor : SceneElementVisitor {
  const SolvableSceneElementVisitor &value_visitor;

  SolvableScenePathVisitor(
    const SolvableSceneElementVisitor &value_visitor
  )
  : SceneElementVisitor(),
    value_visitor(value_visitor)
  {
  }

  void
  visitBodyTranslationComponent(
    Body body, XYZComponent component
  ) override
  {
    value_visitor.visit(bodyTranslationComponent(body.index, component));
  }

  void visitBodyRotationComponent(Body body, XYZComponent component) override
  {
    value_visitor.visit(bodyRotationComponent(body.index, component));
  }

  void visitBodyScale(Body body) override
  {
    value_visitor.visit(BodyScale{body.index});
  }
};
}


void
forSolvableSceneElement(
  const TreePath &path,
  const TreePaths &tree_paths,
  const SolvableSceneElementVisitor &value_visitor
)
{
  SolvableScenePathVisitor visitor = { value_visitor };
  forMatchingScenePath(path, visitor, tree_paths);
}


static const TreePaths::Channel &
channelPaths(
  const Channel &channel,
  const TreePaths &tree_paths
)
{
  struct Visitor : Channel::Visitor {
    const TreePaths &tree_paths;
    const ChannelPaths *&channel_paths_ptr;

    Visitor(const TreePaths &tree_paths, const ChannelPaths *&channel_paths_ptr)
    : tree_paths(tree_paths), channel_paths_ptr(channel_paths_ptr)
    {
    }

    void visit(const BodyTranslationChannel &channel) const override
    {
      channel_paths_ptr = &
        tree_paths.body(bodyOf(channel).index)
        .translation
        .component(channel.component);
    }

    void visit(const BodyRotationChannel &channel) const override
    {
      channel_paths_ptr = &
        tree_paths.body(bodyOf(channel).index)
        .rotation
        .component(channel.component);
    }

    void visit(const BodyScaleChannel &channel) const override
    {
      channel_paths_ptr = &
        tree_paths.body(bodyOf(channel).index)
        .scale;
    }

    void visit(const BodyBoxScaleChannel &channel) const override
    {
      channel_paths_ptr = &
        tree_paths
          .body(bodyOf(channel).index)
          .boxes[bodyBoxOf(channel).index]
          .scale
          .component(channel.component);
    }

    void visit(const BodyBoxCenterChannel &channel) const override
    {
      channel_paths_ptr = &
        tree_paths
          .body(bodyOf(channel).index)
          .boxes[bodyBoxOf(channel).index]
          .center
          .component(channel.component);
    }

    void visit(const MarkerPositionChannel &channel) const override
    {
      channel_paths_ptr =
        &markerPositionComponentChannelPaths(
          markerOf(channel).index, channel.component, tree_paths
        );
    }
  };

  const ChannelPaths *channel_paths_ptr = nullptr;
  channel.accept(Visitor{tree_paths, channel_paths_ptr});
  assert(channel_paths_ptr);
  return *channel_paths_ptr;
}


const TreePath &
channelPath(
  const Channel &channel,
  const TreePaths &tree_paths
)
{
  return channelPaths(channel, tree_paths).path;
}


const TreePath *
channelExpressionPathPtr(
  const Channel &channel,
  const TreePaths &tree_paths
)
{
  return &channelPaths(channel, tree_paths).expression_path;
}


namespace {
struct GeometryDescriber {
  using ItemType = SceneElementDescription::Type;

  bool &found_description;
  const TreePaths::Body &body_paths;
  const TreePath &path;
  SceneElementDescription &description;

  void visitBox(BoxIndex box_index) const
  {
    if (!found_description) {
      const TreePaths::Box &box_paths = body_paths.boxes[box_index];

      if (startsWith(path, box_paths.path)) {
        if (path == box_paths.path) {
          description.type = ItemType::box_geometry;
        }

        description.maybe_box_index = box_index;
        found_description = true;
      }
    }
  }

  void visitBoxes() const
  {
    for (auto i : indicesOf(body_paths.boxes)) {
      visitBox(i);
    }
  }

  void visitLine(LineIndex line_index) const
  {
    if (!found_description) {
      const TreePaths::Line &line_paths = body_paths.lines[line_index];

      if (startsWith(path, line_paths.path)) {
        if (path == line_paths.path) {
          description.type = ItemType::line_geometry;
        }

        description.maybe_line_index = line_index;
        found_description = true;
      }
    }
  }

  void visitLines() const
  {
    for (auto i : indicesOf(body_paths.lines)) {
      visitLine(i);
    }
  }

  void visitMesh(MeshIndex mesh_index) const
  {
    if (found_description) {
      return;
    }

    const TreePaths::Mesh &mesh_paths = body_paths.meshes[mesh_index];

    if (startsWith(path, mesh_paths.path)) {
      description.maybe_mesh_index = mesh_index;

      if (path == mesh_paths.path) {
        description.type = ItemType::mesh_geometry;
      }

      if (startsWith(path, mesh_paths.positions.path)) {
        if (path == mesh_paths.positions.path) {
          description.type = SceneElementDescription::Type::mesh_positions;
        }
        else {
          for (auto i : indicesOf(mesh_paths.positions.elements)) {
            if (startsWith(path, mesh_paths.positions.elements[i].path)) {
              description.maybe_mesh_position_index = i;
              if (path == mesh_paths.positions.elements[i].path) {
                description.type = SceneElementDescription::Type::mesh_position;
              }
            }
          }
        }
      }

      found_description = true;
    }
  }

  void visitMeshes() const
  {
    for (auto i : indicesOf(body_paths.meshes)) {
      visitMesh(i);
    }
  }
};
}


SceneElementDescription
describeTreePath(
  const TreePath &path,
  const TreePaths &tree_paths
)
{
  SceneElementDescription description;
  using ItemType = SceneElementDescription::Type;

  if (path == tree_paths.path) {
    description.type = ItemType::scene;
    return description;
  }

  for (BodyIndex body_index : indicesOf(tree_paths.bodies)) {
    const TreePaths::Body &body_paths = tree_paths.body(body_index);

    if (startsWith(path, body_paths.translation.path)) {
      description.translation_ancestor_function_ptr =
        [](const SceneElementDescription &item)
        {
          assert(item.maybe_body_index);
          SceneElementDescription result;
          result.type = SceneElementDescription::Type::translation;
          result.maybe_body_index = *item.maybe_body_index;
          return result;
        };

      if (path == body_paths.translation.path) {
        description.type = ItemType::translation;
      }

      description.maybe_body_index = body_index;
      return description;
    }

    if (startsWith(path, body_paths.rotation.path)) {
      description.rotation_ancestor_function_ptr =
        [](const SceneElementDescription &item)
        {
          assert(item.maybe_body_index);
          SceneElementDescription result;
          result.type = SceneElementDescription::Type::rotation;
          result.maybe_body_index = *item.maybe_body_index;
          return result;
        };

      description.maybe_body_index = body_index;

      if (path == body_paths.rotation.path) {
        description.type = ItemType::rotation;
      }

      return description;
    }

    if (startsWith(path, body_paths.scale.path)) {
      description.scale_ancestor_function_ptr =
        [](const SceneElementDescription &item)
        {
          assert(item.maybe_body_index);
          SceneElementDescription result;
          result.type = SceneElementDescription::Type::scale;
          result.maybe_body_index = *item.maybe_body_index;
          return result;
        };

      description.maybe_body_index = body_index;

      if (path == body_paths.scale.path) {
        description.type = ItemType::scale;
      }

      return description;
    }

    {
      bool found_description = false;

      GeometryDescriber geometry_describer = {
        found_description,
        body_paths,
        path,
        description
      };

      forEachBodyGeometryType(geometry_describer);

      if (found_description) {
        description.maybe_body_index = body_index;
        return description;
      }
    }

    if (body_paths.path == path) {
      description.type = ItemType::body;
      description.maybe_body_index = body_index;
      return description;
    }
  }

  for (auto i : indicesOf(tree_paths.markers)) {
    const TreePaths::Marker &marker_paths = tree_paths.marker(i);

    if (startsWith(path, marker_paths.path)) {
      description.maybe_marker_index = i;

      if (path == marker_paths.path) {
        description.type = ItemType::marker;
        return description;
      }
    }
  }

  for (auto i : indicesOf(tree_paths.distance_errors)) {
    const TreePaths::DistanceError &distance_error_paths =
      tree_paths.distance_errors[i];

    if (startsWith(path, distance_error_paths.path)) {
      if (path == distance_error_paths.path) {
        description.type = ItemType::distance_error;
        description.maybe_distance_error_index = i;
        return description;
      }

      if (path == distance_error_paths.start) {
        description.type = ItemType::distance_error_start;
        description.maybe_distance_error_index = i;
        return description;
      }

      if (path == distance_error_paths.end) {
        description.type = ItemType::distance_error_end;
        description.maybe_distance_error_index = i;
        return description;
      }
    }
  }

  for (auto i : indicesOf(tree_paths.variables)) {
    if (path == tree_paths.variables[i].path) {
      description.type = ItemType::variable;
      description.maybe_variable_index = i;
      return description;
    }
  }

  return description;
}
