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

using std::cerr;
using std::string;
using LabelProperties = TreeWidget::LabelProperties;
using BoxPaths = TreePaths::Box;
using MarkerPaths = TreePaths::Marker;
using LinePaths = TreePaths::Line;
using BodyState = SceneState::Body;
using BodyPaths = TreePaths::Body;
using ChannelPaths = TreePaths::Channel;


static int defaultDigitsOfPrecision() { return 2; }


static Expression noExpression()
{
  return "";
}


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
    const Expression &expression = noExpression()
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
    const Expression &expression = noExpression()
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


static string markerLabel(const SceneState::Marker &marker_state)
{
  return "[Marker] " + marker_state.name;
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
  const SceneState::Marker &marker_state
)
{
  MarkerPaths marker_paths;
  marker_paths.path = path;
  const string &name = marker_state.name;
  tree_widget.createVoidItem(path,LabelProperties{markerLabel(marker_state)});
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

  if (maybe_body_index) {
    int n_boxes = tree_paths.body(*maybe_body_index).boxes.size();
    index += n_boxes;
  }
  else {
    // Can't add boxes to the scene
  }

  result.box_path = childPath(body_path, index);

  if (maybe_body_index) {
    int n_lines = tree_paths.body(*maybe_body_index).lines.size();
    index += n_lines;
  }
  else {
    // Can't add lines to the scene
  }

  result.line_path = childPath(body_path, index);
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
    createMarker(tree_widget, marker_path, state_marker);
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

  size_t n_boxes = body_state.boxes.size();
  body_paths.boxes.resize(n_boxes);

  for (size_t i=0; i!=n_boxes; ++i) {
    TreePath box_path = childPath(adder.parent_path, adder.n_children);

    body_paths.boxes[i] =
      createBoxItem(box_path, tree_widget, body_state.boxes[i]);

    ++adder.n_children;
  }

  size_t n_lines = body_state.lines.size();
  body_paths.lines.resize(n_lines);

  for (size_t i=0; i!=n_lines; ++i) {
    TreePath line_path = childPath(adder.parent_path, adder.n_children);

    body_paths.lines[i] =
      createLineItem(line_path, tree_widget, body_state.lines[i]);

    ++adder.n_children;
  }

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
    updateXYZValues(tree_widget, translation_paths, vec3(translation));
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
  size_t n_boxes = body_state.boxes.size();

  for (size_t i=0; i!=n_boxes; ++i) {
    const BoxPaths &box_paths = body_paths.boxes[i];
    const SceneState::Box &box_state = body_state.boxes[i];
    {
      const TreePaths::XYZChannels &scale_paths = box_paths.scale;
      const SceneState::XYZ &scale = box_state.scale;
      updateXYZValues(tree_widget, scale_paths, vec3(scale));
    }
    {
      const TreePaths::XYZChannels &center_paths = box_paths.center;
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

  for (auto i : indicesOf(tree_paths.variables)) {
    updateVariable(i, tree_widget, tree_paths, state);
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


void
setTreeBoolValue(
  TreeWidget &tree_widget,
  const TreePath &path,
  bool new_state
)
{
  tree_widget.setItemBoolValue(path, new_state);
}


namespace {
struct NumericComponentVisitor {
  virtual bool visitComponent(NumericValue &value_to_set) = 0;
};
}


namespace {
struct SetNumericComponentVisitor : NumericComponentVisitor {
  const NumericValue value;

  SetNumericComponentVisitor(NumericValue value)
  : value(value)
  {
  }

  bool visitComponent(NumericValue &value_to_set) override
  {
    value_to_set = value;
    return true;
  }
};
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


static bool
visitComponentOf(
  NumericComponentVisitor &visitor,
  XYZComponent component,
  SceneState::XYZ &v
)
{
  switch (component) {
    case XYZComponent::x: return visitor.visitComponent(v.x);
    case XYZComponent::y: return visitor.visitComponent(v.y);
    case XYZComponent::z: return visitor.visitComponent(v.z);
  }

  assert(false); // not implemented
  return false;
}


template <typename Component>
static bool
forMatchingComponent(
  SceneState::XYZ &xyz_state,
  const TreePath &path,
  const TreePaths::BasicXYZ<Component> &xyz_path,
  NumericComponentVisitor &visitor
)
{
  XYZComponent component = matchingComponent(path, xyz_path);
  return visitComponentOf(visitor, component, xyz_state);
}


namespace {
struct SceneElementVisitor {
  virtual bool visitBody(BodyIndex) = 0;
  virtual bool visitMarker(MarkerIndex) = 0;
  virtual bool visitDistanceError(DistanceErrorIndex) = 0;
  virtual bool visitVariable(VariableIndex) = 0;
  virtual bool visitBodyName(BodyIndex) { return false; }
  virtual bool visitBodyTranslation(BodyIndex) { return false; }
  virtual bool visitBodyRotation(BodyIndex) { return false; }
  virtual bool visitBodyBox(BodyIndex, BoxIndex) { return false; }
  virtual bool visitBodyLine(BodyIndex, LineIndex) { return false; }
  virtual bool visitMarkerName(MarkerIndex) { return false; }
};
}


namespace {
struct ScenePathVisitor : SceneElementVisitor {
  const TreePaths &tree_paths;
  const TreePath &path;

  ScenePathVisitor(const TreePaths &tree_paths, const TreePath &path)
  : tree_paths(tree_paths),
    path(path)
  {
  }

  virtual bool
  visitBodyBoxScaleComponent(BodyIndex, BoxIndex, XYZComponent)
  {
    return false;
  }

  virtual bool
  visitBodyBoxScale(BodyIndex body_index, BoxIndex box_index)
  {
    const TreePaths::Body &body_paths = tree_paths.body(body_index);
    const BoxPaths &box_paths = body_paths.boxes[box_index];
    XYZComponent component = matchingComponent(path, box_paths.scale);
    return visitBodyBoxScaleComponent(body_index, box_index, component);
  }

  virtual bool visitBodyBoxCenter(BodyIndex, BoxIndex) { return false; }

  virtual bool
  visitBodyTranslationComponent(BodyIndex, XYZComponent)
  {
    return false;
  }

  virtual bool
  visitBodyRotationComponent(BodyIndex, XYZComponent)
  {
    return false;
  }

  virtual bool visitBodyScale(BodyIndex)
  {
    return false;
  }

  bool visitBodyTranslation(BodyIndex body_index) override
  {
    XYZComponent component =
      matchingComponent(path, tree_paths.body(body_index).translation);

    return visitBodyTranslationComponent(body_index, component);
  }

  bool visitBodyRotation(BodyIndex body_index) override
  {
    XYZComponent component =
      matchingComponent(path, tree_paths.body(body_index).rotation);

    return visitBodyRotationComponent(body_index, component);
  }

  virtual bool visitBodyBox(BodyIndex body_index, BoxIndex box_index)
  {
    const TreePaths::Body &body_paths = tree_paths.body(body_index);
    const BoxPaths &box_paths = body_paths.boxes[box_index];

    if (startsWith(path, box_paths.scale.path)) {
      return visitBodyBoxScale(body_index, box_index);
    }
    else if (startsWith(path, box_paths.center.path)) {
      return visitBodyBoxCenter(body_index, box_index);
    }
    else {
      assert(false); // not implemented
    }

    return false;
  }

  bool visitBody(BodyIndex body_index) override
  {
    const TreePaths::Body &body_paths = tree_paths.body(body_index);

    if (path == body_paths.name) {
      return visitBodyName(body_index);
    }

    if (startsWith(path, body_paths.translation.path)) {
      return visitBodyTranslation(body_index);
    }

    if (startsWith(path, body_paths.rotation.path)) {
      return visitBodyRotation(body_index);
    }

    if (isMatchingPath(path, body_paths.scale)) {
      return visitBodyScale(body_index);
    }

    BoxIndex n_boxes = body_paths.boxes.size();

    for (BoxIndex box_index = 0; box_index != n_boxes; ++box_index) {
      if (startsWith(path, body_paths.boxes[box_index].path)) {
        return visitBodyBox(body_index, box_index);
      }
    }

    LineIndex n_lines = body_paths.lines.size();

    for (LineIndex line_index = 0; line_index != n_lines; ++line_index) {
      if (startsWith(path, body_paths.lines[line_index].path)) {
        return visitBodyLine(body_index, line_index);
      }
    }

    return false;
  }

  bool visitMarker(MarkerIndex marker_index) override
  {
    const MarkerPaths &marker_paths = tree_paths.marker(marker_index);

    if (startsWith(path, marker_paths.position.path)) {
      return visitMarkerPosition(marker_index);
    }

    if (startsWith(path, marker_paths.name)) {
      return visitMarkerName(marker_index);
    }

    return false;
  }

  bool visitDistanceError(DistanceErrorIndex distance_error_index) override
  {
    const TreePaths::DistanceError &distance_error_paths =
      tree_paths.distance_errors[distance_error_index];

    if (startsWith(path, distance_error_paths.desired_distance)) {
      return visitDistanceErrorDesiredDistance(distance_error_index);
    }

    if (startsWith(path, distance_error_paths.weight)) {
      return visitDistanceErrorWeight(distance_error_index);
    }

    return false;
  }

  bool visitVariable(VariableIndex variable_index) override
  {
    const TreePaths::Variable &variable_paths =
      tree_paths.variables[variable_index];

    if (path == variable_paths.path) {
      return visitVariableValue(variable_index);
    }

    if (startsWith(path, variable_paths.name)) {
      return visitVariableName(variable_index);
    }

    return false;
  }

  virtual bool visitMarkerPosition(MarkerIndex)
  {
    return false;
  }

  virtual bool visitVariableValue(VariableIndex)
  {
    return false;
  }

  virtual bool visitVariableName(VariableIndex)
  {
    return false;
  }

  virtual bool visitDistanceErrorDesiredDistance(DistanceErrorIndex)
  {
    return false;
  }

  virtual bool visitDistanceErrorWeight(DistanceErrorIndex)
  {
    return false;
  }
};
}


namespace {
struct SetStringValueVisitor : ScenePathVisitor {
  SceneState &scene_state;
  const StringValue &value;

  SetStringValueVisitor(
    SceneState &scene_state,
    const TreePaths &tree_paths,
    const StringValue &value,
    const TreePath &path
  )
  : ScenePathVisitor(tree_paths, path),
    scene_state(scene_state),
    value(value)
  {
  }

  bool visitBodyName(BodyIndex body_index) override
  {
    SceneState::Body &body_state = scene_state.body(body_index);

    if (!findBodyIndex(scene_state, value)) {
      body_state.name = value;
    }
    else {
      // Can't allow duplicate body names.
    }

    return true;
  }

  bool visitMarkerName(MarkerIndex marker_index) override
  {
    SceneState::Marker &marker_state = scene_state.marker(marker_index);

    if (!findMarkerIndex(scene_state, value)) {
      marker_state.name = value;
    }
    else {
      // Can't allow duplicate marker names.
    }

    return true;
  }

  bool visitVariableName(VariableIndex variable_index) override
  {
    SceneState::Variable &variable_state =
      scene_state.variables[variable_index];

    if (!findVariableIndex(scene_state, value)) {
      variable_state.name = value;
    }
    else {
      // Can't allow duplicate variable names.
    }

    return true;
  }
};
}


namespace {
struct SetNumericValueVisitor : ScenePathVisitor {
  SceneState &scene_state;
  SetNumericComponentVisitor visitor;

  SetNumericValueVisitor(
    SceneState &scene_state,
    const TreePaths &tree_paths,
    const TreePath &path,
    NumericValue value
  )
  : ScenePathVisitor(tree_paths, path),
    scene_state(scene_state),
    visitor{value}
  {
  }

  bool
  visitBodyTranslationComponent(
    BodyIndex body_index,
    XYZComponent component
  ) override
  {
    return
      visitComponentOf(
        visitor,
        component,
        scene_state.body(body_index).transform.translation
      );
  }

  bool
  visitBodyRotationComponent(
    BodyIndex body_index,
    XYZComponent component
  ) override
  {
    return
      visitComponentOf(
        visitor,
        component,
        scene_state.body(body_index).transform.rotation
      );
  }

  bool visitBodyScale(BodyIndex body_index) override
  {
    scene_state.body(body_index).transform.scale = visitor.value;
    return true;
  }

  bool
  visitBodyBoxScaleComponent(
    BodyIndex body_index, BoxIndex box_index, XYZComponent component
  ) override
  {
    SceneState::Body &body_state = scene_state.body(body_index);
    SceneState::Box &box_state = body_state.boxes[box_index];
    return visitComponentOf(visitor, component, box_state.scale);
  }

  bool visitBodyBoxCenter(BodyIndex body_index, BoxIndex box_index) override
  {
    const TreePaths::Body &body_paths = tree_paths.body(body_index);
    const BoxPaths &box_paths = body_paths.boxes[box_index];
    SceneState::Body &body_state = scene_state.body(body_index);
    SceneState::Box &box_state = body_state.boxes[box_index];

    return
      forMatchingComponent(
        box_state.center, path, box_paths.center, visitor
      );
  }

  bool visitBodyLine(BodyIndex body_index, LineIndex line_index) override
  {
    SceneState::Body &body_state = scene_state.body(body_index);
    const TreePaths::Body &body_paths = tree_paths.body(body_index);
    const LinePaths &line_paths = body_paths.lines[line_index];
    SceneState::Line &line_state = body_state.lines[line_index];

    if (startsWith(path, line_paths.start.path)) {
      return
        forMatchingComponent(
          line_state.start, path, line_paths.start, visitor
        );
    }

    if (startsWith(path, line_paths.end.path)) {
      return
        forMatchingComponent(
          line_state.end, path, line_paths.end, visitor
        );
    }

    return false;
  }

  bool visitMarkerPosition(MarkerIndex marker_index) override
  {
    const MarkerPaths &marker_paths = tree_paths.marker(marker_index);
    SceneState::Marker &marker_state = scene_state.marker(marker_index);

    return
      forMatchingComponent(
        marker_state.position, path, marker_paths.position, visitor
      );
  }

  bool
  visitDistanceErrorDesiredDistance(
    DistanceErrorIndex distance_error_index
  ) override
  {
    SceneState::DistanceError &distance_error_state =
      scene_state.distance_errors[distance_error_index];

    distance_error_state.desired_distance = visitor.value;
    return true;
  }

  bool
  visitDistanceErrorWeight(DistanceErrorIndex distance_error_index) override
  {
    SceneState::DistanceError &distance_error_state =
      scene_state.distance_errors[distance_error_index];

    distance_error_state.weight = visitor.value;
    return true;
  }

  bool visitVariableValue(VariableIndex variable_index) override
  {
    SceneState::Variable &variable_state =
      scene_state.variables[variable_index];

    variable_state.value = visitor.value;
    return true;
  }
};
}


namespace {
struct PathChannelVisitor : ScenePathVisitor {
  const std::function<void(const Channel &)> &channel_function;

  PathChannelVisitor(
    const TreePaths &tree_paths,
    const TreePath &path,
    const std::function<void(const Channel &)> &channel_function
  )
  : ScenePathVisitor(tree_paths, path),
    channel_function(channel_function)
  {
  }

  void visitChannel(const Channel &channel)
  {
    channel_function(channel);
  }

  bool
  visitBodyBoxScaleComponent(
    BodyIndex body_index,
    BoxIndex box_index,
    XYZComponent component
  ) override
  {
    visitChannel(BodyBoxScaleChannel(body_index, box_index, component));
    return true;
  }

  bool
  visitBodyTranslationComponent(
    BodyIndex body_index,
    XYZComponent component
  ) override
  {
    visitChannel(BodyTranslationChannel(body_index, component));
    return true;
  }

  bool
  visitBodyRotationComponent(
    BodyIndex body_index,
    XYZComponent component
  ) override
  {
    visitChannel(BodyRotationChannel(body_index, component));
    return true;
  }
};
}


static bool
forMatchingScenePath(
  const TreePath &path,
  SceneElementVisitor &visitor,
  const TreePaths &tree_paths
)
{
  for (auto i : indicesOf(tree_paths.markers)) {
    if (startsWith(path, tree_paths.marker(i).path)) {
      return visitor.visitMarker(i);
    }
  }

  for (auto i : indicesOf(tree_paths.bodies)) {
    if (startsWith(path, tree_paths.body(i).path)) {
      if (visitor.visitBody(i)) {
        return true;
      }
    }
  }

  for (auto i : indicesOf(tree_paths.distance_errors)) {
    if (startsWith(path, tree_paths.distance_errors[i].path)) {
      return visitor.visitDistanceError(i);
    }
  }

  for (auto i : indicesOf(tree_paths.variables)) {
    const TreePaths::Variable &variable_paths = tree_paths.variables[i];

    if (startsWith(path, variable_paths.path)) {
      return visitor.visitVariable(i);
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
  SetNumericValueVisitor visitor = {scene_state, tree_paths, path, value};
  return forMatchingScenePath(path, visitor, tree_paths);
}


bool
setSceneStateStringValue(
  SceneState &scene_state,
  const TreePath &path,
  const StringValue &value,
  const TreePaths &tree_paths
)
{
  SetStringValueVisitor visitor{scene_state, tree_paths, value, path};
  return forMatchingScenePath(path, visitor, tree_paths);
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



// Seems like we need a SolvableSceneValueVisitor
// This would make it more obvious which things need to be solved.
namespace {
struct SolvableScenePathVisitor : ScenePathVisitor {
  const SolvableSceneValueVisitor &value_visitor;

  SolvableScenePathVisitor(
    const TreePaths &paths,
    const TreePath &path,
    const SolvableSceneValueVisitor &value_visitor
  )
  : ScenePathVisitor(paths, path),
    value_visitor(value_visitor)
  {
  }

  bool
  visitBodyTranslationComponent(
    BodyIndex body_index, XYZComponent component
  ) override
  {
    value_visitor.visitBodyTranslationComponent(body_index, component);
    return true;
  }

  bool
  visitBodyRotationComponent(
    BodyIndex body_index, XYZComponent component
  ) override
  {
    value_visitor.visitBodyRotationComponent(body_index, component);
    return true;
  }

  bool visitBodyScale(BodyIndex body_index) override
  {
    value_visitor.visitBodyScale(body_index);
    return true;
  }
};
}


void
forSolvableSceneValue(
  const TreePath &path,
  const TreePaths &tree_paths,
  const SolvableSceneValueVisitor &value_visitor
)
{
  SolvableScenePathVisitor visitor = {
    tree_paths, path, value_visitor
  };

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
        tree_paths.body(channel.body_index)
        .translation
        .component(channel.component);
    }

    void visit(const BodyRotationChannel &channel) const override
    {
      channel_paths_ptr = &
        tree_paths.body(channel.body_index)
        .rotation
        .component(channel.component);
    }

    void visit(const BodyScaleChannel &channel) const override
    {
      channel_paths_ptr = &
        tree_paths.body(channel.body_index)
        .scale;
    }

    virtual void visit(const BodyBoxScaleChannel &channel) const
    {
      channel_paths_ptr = &
        tree_paths
          .body(channel.body_index)
          .boxes[channel.box_index]
          .scale
          .component(channel.component);
    }

    virtual void visit(const BodyBoxCenterChannel &channel) const
    {
      channel_paths_ptr = &
        tree_paths
          .body(channel.body_index)
          .boxes[channel.box_index]
          .center
          .component(channel.component);
    }

    virtual void visit(const MarkerPositionChannel &channel) const
    {
      channel_paths_ptr =
        &markerPositionComponentChannelPaths(
          channel.marker_index, channel.component, tree_paths
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
