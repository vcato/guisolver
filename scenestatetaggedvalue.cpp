#include "scenestatetaggedvalue.hpp"

#include <sstream>
#include "indicesof.hpp"
#include "contains.hpp"
#include "nextunusedname.hpp"
#include "taggedvalueio.hpp"
#include "pointlink.hpp"

using std::string;
using std::cerr;
using std::istringstream;
using std::ostringstream;
using XYZChannels = SceneState::XYZChannelsRef;
using TransformSolveFlags = SceneState::TransformSolveFlags;
using TransformExpressions = SceneState::TransformExpressions;


static void
resolveMarkerNameConflicts(
  SceneState &scene_state,
  MarkerNameMap &marker_name_map
)
{
  for (MarkerIndex marker_index : indicesOf(scene_state.markers())) {
    std::string &name = scene_state.marker(marker_index).name;

    if (name[0] == '$') {
      const string &old_name = name.substr(1);
      name = nextUnusedName(markerNames(scene_state), old_name + "_");
      marker_name_map[old_name] = name;
    }
  }
}


static string withoutTrailingNumber(const string &arg)
{
  string::const_iterator i = arg.end();

  while (i != arg.begin() && isdigit(*(i-1))) --i;

  return string(arg.begin(), i);
}


static void
resolveBodyNameConflicts(BodyIndex body_index, SceneState &scene_state)
{
  std::string &name = scene_state.body(body_index).name;

  if (name[0] == '$') {
    string prefix = withoutTrailingNumber(name.substr(1));
    name = nextUnusedName(bodyNames(scene_state), prefix);
  }
}


static NumericValue
  childNumericValueOr(
    const TaggedValue &tagged_value,
    const TaggedValue::Tag &child_name,
    NumericValue default_value
  )
{
  return findNumericValue(tagged_value, child_name).valueOr(default_value);
}


static int intValueOr(const TaggedValue &tv, int /*default_value*/)
{
  const NumericValue *numeric_ptr = tv.value.maybeNumeric();

  if (!numeric_ptr) {
    assert(false); // not implemented
  }
  else {
    NumericValue nv = *numeric_ptr;

    if (int(nv) != nv) {
      assert(false); // not implemented
    }

    return int(nv);
  }
}


static int
childIntValueOr(
  const TaggedValue &tagged_value,
  const string &tag,
  int default_value
)
{
  const TaggedValue *child_ptr = findChild(tagged_value, tag);

  if (child_ptr) {
    return intValueOr(*child_ptr, default_value);
  }

  return default_value;
}


static bool
  childBoolValueOr(
    const TaggedValue &tagged_value,
    const TaggedValue::Tag &child_name,
    bool default_value
  )
{
  return findBoolValue(tagged_value, child_name).valueOr(default_value);
}


static string
  childStringValueOr(
    const TaggedValue &tagged_value,
    const TaggedValue::Tag &child_name,
    const string &default_value
  )
{
  return findStringValue(tagged_value, child_name).valueOr(default_value);
}


static SceneState::XYZ xyzStateFromTaggedValue(const TaggedValue &tagged_value)
{
  NumericValue x = childNumericValueOr(tagged_value, "x", 0);
  NumericValue y = childNumericValueOr(tagged_value, "y", 0);
  NumericValue z = childNumericValueOr(tagged_value, "z", 0);

  return {x,y,z};
}


static Expression
expressionFor(
  const TaggedValue &xyz_tagged_value,
  const string &child_name
)
{
  const TaggedValue *child_ptr = findChild(xyz_tagged_value, child_name);

  if (!child_ptr) {
    return "";
  }

  return childStringValueOr(*child_ptr, "expression", "");
}


static SceneState::XYZExpressions
xyzExpressionsFromTaggedValue(const TaggedValue &tagged_value)
{
  Expression x = expressionFor(tagged_value, "x");
  Expression y = expressionFor(tagged_value, "y");
  Expression z = expressionFor(tagged_value, "z");

  return {x,y,z};
}


static TransformState
makeTransformFromTaggedValue(const TaggedValue &tagged_value)
{
  TransformState result;

  const TaggedValue *translation_ptr = findChild(tagged_value, "translation");

  if (!translation_ptr) {
    assert(false);
  }

  const TaggedValue *rotation_ptr = findChild(tagged_value, "rotation");

  if (!rotation_ptr) {
    assert(false);
  }

  const auto default_scale = SceneState::Transform::defaultScale();

  result.translation = xyzStateFromTaggedValue(*translation_ptr);
  result.rotation = xyzStateFromTaggedValue(*rotation_ptr);
  result.scale = findNumericValue(tagged_value, "scale").valueOr(default_scale);

  return result;
}


static bool
solveFlagFor(
  const TaggedValue &xyz_tagged_value,
  const string &child_name
)
{
  const TaggedValue *child_ptr = findChild(xyz_tagged_value, child_name);

  if (!child_ptr) {
    assert(false); // not implemented
  }

  return childBoolValueOr(*child_ptr, "solve", true);
}


static SceneState::XYZSolveFlags
  makeXYZSolveFlagsFromTaggedValue(
    const TaggedValue &tagged_value,
    const string &child_name
  )
{
  const TaggedValue *child_ptr = findChild(tagged_value, child_name);

  if (!child_ptr) {
    assert(false); // not implemented
  }

  SceneState::XYZSolveFlags flags;

  flags.x = solveFlagFor(*child_ptr, "x");
  flags.y = solveFlagFor(*child_ptr, "y");
  flags.z = solveFlagFor(*child_ptr, "z");

  return flags;
}


static SceneState::XYZExpressions
  makeXYZExpressionsFromTaggedValue(
    const TaggedValue &tagged_value,
    const string &child_name
  )
{
  const TaggedValue *child_ptr = findChild(tagged_value, child_name);

  if (!child_ptr) {
    assert(false); // not implemented
  }

  return xyzExpressionsFromTaggedValue(*child_ptr);
}


static TransformSolveFlags
  makeSolveFlagsFromTaggedValue(const TaggedValue &tagged_value)
{
  TransformSolveFlags result;

  result.translation =
    makeXYZSolveFlagsFromTaggedValue(tagged_value, "translation");

  result.rotation =
    makeXYZSolveFlagsFromTaggedValue(tagged_value, "rotation");

  return result;
}


static TransformExpressions
makeExpressionsFromTaggedValue(const TaggedValue &tagged_value)
{
  TransformExpressions result;

  result.translation =
    makeXYZExpressionsFromTaggedValue(tagged_value, "translation");

  result.rotation =
    makeXYZExpressionsFromTaggedValue(tagged_value, "rotation");

  result.scale = expressionFor(tagged_value, "scale");

  return result;
}


static SceneState::XYZ
  xyzValueOr(const TaggedValue &tv, const SceneState::XYZ &default_value)
{
  SceneState::XYZ result;
  result.x = childNumericValueOr(tv, "x", default_value.x);
  result.y = childNumericValueOr(tv, "y", default_value.y);
  result.z = childNumericValueOr(tv, "z", default_value.z);
  return result;
}


static bool
markerNameExists(
  const SceneState::Marker::Name &name,
  const SceneState &scene_state
)
{
  return contains(markerNames(scene_state), name);
}


static bool
bodyNameExists(
  const SceneState::Marker::Name &name,
  const SceneState &scene_state
)
{
  return contains(bodyNames(scene_state), name);
}


static MarkerIndex
createMarkerFromTaggedValueWithoutResolvingConflicts(
  SceneState &scene_state,
  const TaggedValue &tagged_value,
  Optional<BodyIndex> maybe_parent_index
)
{
  Optional<StringValue> maybe_old_name =
    findStringValue(tagged_value, "name");

  Optional<StringValue> maybe_name;

  if (maybe_old_name) {
    if (!markerNameExists(*maybe_old_name, scene_state)) {
      maybe_name = maybe_old_name;
    }
    else {
      maybe_name = "$" + *maybe_old_name;
    }
  }

  MarkerIndex marker_index = scene_state.createMarker(maybe_parent_index);

  if (maybe_name) {
    scene_state.marker(marker_index).name = *maybe_name;
  }

  const TaggedValue *position_ptr = findChild(tagged_value, "position");

  if (position_ptr) {
    scene_state.marker(marker_index).position =
      xyzStateFromTaggedValue(*position_ptr);

    scene_state.marker(marker_index).position_expressions =
      xyzExpressionsFromTaggedValue(*position_ptr);
  }

  return marker_index;
}


namespace {
struct BodyTaggedValue {
  Optional<BodyIndex> maybe_body_index;
  TaggedValue child_tagged_value;
};
}


static void
addMarkerTaggedValues(
  const TaggedValue &tagged_value,
  Optional<BodyIndex> maybe_parent_index,
  vector<BodyTaggedValue> &body_tagged_values
)
{
  for (auto &child_tagged_value : tagged_value.children) {
    if (child_tagged_value.tag == "Marker") {
      body_tagged_values.push_back({maybe_parent_index, child_tagged_value});
    }
  }
}


static void
createMarkersFromTaggedValues(
  const vector<BodyTaggedValue> &body_tagged_values,
  SceneState &scene_state
)
{
  for (const BodyTaggedValue &body_tagged_value : body_tagged_values) {
    const TaggedValue &child_tagged_value =
      body_tagged_value.child_tagged_value;

    if (child_tagged_value.tag == "Marker") {
      const Optional<BodyIndex> maybe_parent_index =
        body_tagged_value.maybe_body_index;

      createMarkerFromTaggedValueWithoutResolvingConflicts(
        scene_state,
        child_tagged_value,
        maybe_parent_index
      );
    }
  }
}


// This one
static SceneState::XYZ
childXYZValueOr(
  const TaggedValue &tagged_value,
  const string &tag,
  const SceneState::XYZ &default_value
)
{
  const TaggedValue *child_ptr = findChild(tagged_value, tag);

  if (child_ptr) {
    return xyzValueOr(*child_ptr, default_value);
  }

  return default_value;
}


static void
fillBoxStateFromTaggedValue(
  SceneState::Box &box_state,
  const TaggedValue &box_tagged_value
)
{
  {
    const TaggedValue *scale_ptr = findChild(box_tagged_value, "scale");
    const SceneState::XYZ default_scale = {1,1,1};

    if (scale_ptr) {
      box_state.scale = xyzValueOr(*scale_ptr, default_scale);

      box_state.scale_expressions =
        xyzExpressionsFromTaggedValue(*scale_ptr);
    }
    else {
      SceneState::XYZ box_scale;

      box_scale.x =
        childNumericValueOr(box_tagged_value, "scale_x", default_scale.x);

      box_scale.y =
        childNumericValueOr(box_tagged_value, "scale_y", default_scale.y);

      box_scale.z =
        childNumericValueOr(box_tagged_value, "scale_z", default_scale.z);

      box_state.scale = box_scale;
    }
  }

  box_state.center = childXYZValueOr(box_tagged_value, "center", {0,0,0});

  box_state.center_expressions =
    makeXYZExpressionsFromTaggedValue(box_tagged_value, "center");
}


static void
fillLineStateFromTaggedValue(
  SceneState::Line &line_state,
  const TaggedValue &line_tagged_value
)
{
  line_state.start = childXYZValueOr(line_tagged_value, "start", {0,0,0});
  line_state.end = childXYZValueOr(line_tagged_value, "end", {1,0,0});
}


static bool
isValidPositionIndex(int v, int n_positions)
{
  if (v < 0) {
    assert(false); // not implemented
  }

  if (v >= n_positions) {
    assert(false); // not implemented
  }

  return true;
}


static bool
isValidTriangle(int v1, int v2, int v3, int n_positions)
{
  return
    isValidPositionIndex(v1, n_positions) &&
    isValidPositionIndex(v2, n_positions) &&
    isValidPositionIndex(v3, n_positions) &&
    v1 != v2 &&
    v2 != v3 &&
    v3 != v1;
}


static void
fillMeshStateFromTaggedValue(
  SceneState::Mesh &mesh_state,
  const TaggedValue &mesh_tagged_value
)
{
  //printTaggedValueOn(cerr, mesh_tagged_value);
  mesh_state.scale = childXYZValueOr(mesh_tagged_value, "scale", {1,1,1});
  mesh_state.center = childXYZValueOr(mesh_tagged_value, "center", {0,0,0});

  const TaggedValue *positions_ptr = findChild(mesh_tagged_value, "positions");
  auto &positions_state = mesh_state.shape.positions;

  if (positions_ptr) {
    for (auto &child_tagged_value : positions_ptr->children) {
      auto &index_string = child_tagged_value.tag;
      istringstream stream(index_string);

      int index = 0;
      stream >> index;

      if (!stream) {
        assert(false); // not implemented
      }

      if (index < 0) {
        assert(false); // not implemented
      }

      if (int(positions_state.size()) <= index) {
        positions_state.resize(index + 1, SceneState::XYZ{0,0,0});
      }

      positions_state[index] = xyzValueOr(child_tagged_value, {0,0,0});
    }
  }

  for (auto &child_tagged_value : mesh_tagged_value.children) {
    if (child_tagged_value.tag == "Triangle") {
      int v1 = childIntValueOr(child_tagged_value, "vertex1", 0);
      int v2 = childIntValueOr(child_tagged_value, "vertex2", 0);
      int v3 = childIntValueOr(child_tagged_value, "vertex3", 0);
      int n_positions = positions_state.size();

      if (!isValidTriangle(v1, v2, v3, n_positions)) {
        assert(false); // not implemented
      }

      mesh_state.shape.triangles.emplace_back(v1, v2, v3);
    }
  }
}


static const StringValue &
mappedMarkerName(
  const StringValue &marker_name,
  const MarkerNameMap &marker_name_map
)
{
  auto iter = marker_name_map.find(marker_name);

  if (iter == marker_name_map.end()) {
    return marker_name;
  }
  else {
    return iter->second;
  }
}


static void
setDistanceErrorMarkerMember(
  SceneState::DistanceError &distance_error_state,
  void (SceneState::DistanceError::* member_ptr)(Optional<Marker>),
  const StringValue &marker_name,
  const MarkerNameMap &marker_name_map,
  SceneState &result
)
{
  StringValue mapped_marker_name =
    mappedMarkerName(marker_name, marker_name_map);

  (distance_error_state.*member_ptr)(
    makeMarker(findMarkerWithName(result, mapped_marker_name))
  );
}


static void
scanPointRef(
  const std::string &tag,
  SceneState::DistanceError &distance_error_state,
  void (SceneState::DistanceError::* member_ptr)(Optional<Marker>),
  const TaggedValue &tagged_value,
  const MarkerNameMap &marker_name_map,
  SceneState &result
)
{
  const TaggedValue::Tag &child_name = tag;
  const TaggedValue *child_ptr = findChild(tagged_value, child_name);

  if (!child_ptr) {
    return;
  }

  const TaggedValue &child = *child_ptr;
  const PrimaryValue &value = child.value;

  if (const StringValue *string_ptr = value.maybeString()) {
    const StringValue &marker_name = *string_ptr;

    setDistanceErrorMarkerMember(
      distance_error_state, member_ptr, marker_name, marker_name_map, result
    );
  }
  else if (const EnumerationValue *enum_ptr = value.maybeEnumeration()) {
    if (enum_ptr->name == "MarkerRef") {
      Optional<StringValue> maybe_marker_name =
        findStringValue(child, "marker_name");

      if (maybe_marker_name) {
        setDistanceErrorMarkerMember(
          distance_error_state,
          member_ptr,
          *maybe_marker_name,
          marker_name_map,
          result
        );
      }
    }
  }
}


static void
createDistanceErrorFromTaggedValue(
  SceneState &result,
  const Optional<BodyIndex> maybe_body_index,
  const TaggedValue &tagged_value,
  const MarkerNameMap &marker_name_map
)
{
  DistanceErrorIndex index = result.createDistanceError(maybe_body_index);

  SceneState::DistanceError &distance_error_state =
    result.distance_errors[index];

  scanPointRef(
    "start",
    distance_error_state,
    &SceneState::DistanceError::setStart,
    tagged_value,
    marker_name_map,
    result
  );

  scanPointRef(
    "end",
    distance_error_state,
    &SceneState::DistanceError::setEnd,
    tagged_value,
    marker_name_map,
    result
  );

  {
    auto tag = "desired_distance";
    auto optional_value = findNumericValue(tagged_value, tag);

    if (optional_value) {
      distance_error_state.desired_distance = *optional_value;
    }
  }

  {
    auto tag = "weight";
    auto optional_value = findNumericValue(tagged_value, tag);

    if (optional_value) {
      distance_error_state.weight = *optional_value;
    }
  }
}


static void
buildDistanceErrorTaggedValues(
  const TaggedValue &tagged_value,
  Optional<BodyIndex> maybe_body_index,
  vector<BodyTaggedValue> &body_tagged_values
)
{
  for (auto &child_tagged_value : tagged_value.children) {
    if (child_tagged_value.tag == "DistanceError") {
      body_tagged_values.push_back({maybe_body_index, child_tagged_value});
    }
  }
}


static void
createDistanceErrorsFromTaggedValues(
  const vector<BodyTaggedValue> &body_tagged_values,
  SceneState &result,
  const MarkerNameMap &marker_name_map
)
{
  for (auto &body_tagged_value : body_tagged_values) {
    if (body_tagged_value.child_tagged_value.tag == "DistanceError") {
      Optional<BodyIndex> maybe_body_index =
        body_tagged_value.maybe_body_index;

      const TaggedValue &child_tagged_value =
        body_tagged_value.child_tagged_value;

      createDistanceErrorFromTaggedValue(
        result, maybe_body_index, child_tagged_value, marker_name_map
      );
    }
  }
}


static void
createChildBodiesInSceneState(
  SceneState &result,
  const TaggedValue &tagged_value,
  const Optional<BodyIndex> maybe_parent_index,
  vector<BodyTaggedValue> &body_tagged_values
);


static BodyIndex
createBodyInSceneState(
  SceneState &result,
  const TaggedValue &tagged_value,
  const Optional<BodyIndex> maybe_parent_index
)
{
  Optional<StringValue> maybe_old_name =
    findStringValue(tagged_value, "name");

  Optional<StringValue> maybe_name;

  if (maybe_old_name) {
    if (!bodyNameExists(*maybe_old_name, result)) {
      maybe_name = maybe_old_name;
    }
    else {
      maybe_name = "$" + *maybe_old_name;
    }
  }

  BodyIndex body_index = result.createBody(maybe_parent_index);

  SceneState::Body &body_state = result.body(body_index);

  if (maybe_name) {
    body_state.name = *maybe_name;
  }

  setAll(body_state.solve_flags, true);

  body_state.transform =
    makeTransformFromTaggedValue(tagged_value);

  body_state.solve_flags =
    makeSolveFlagsFromTaggedValue(tagged_value);

  body_state.expressions =
    makeExpressionsFromTaggedValue(tagged_value);

  for (auto &child : tagged_value.children) {
    if (child.tag == "Box") {
      const TaggedValue &box_tagged_value = child;
      BoxIndex box_index = body_state.createBox();
      SceneState::Box &box_state = body_state.boxes[box_index];
      fillBoxStateFromTaggedValue(box_state, box_tagged_value);
    }
  }

  for (auto &child : tagged_value.children) {
    if (child.tag == "Line") {
      const TaggedValue &line_tagged_value = child;
      LineIndex line_index = body_state.createLine();
      SceneState::Line &line_state = body_state.lines[line_index];
      fillLineStateFromTaggedValue(line_state, line_tagged_value);
    }
  }

  for (auto &child : tagged_value.children) {
    if (child.tag == "Mesh") {
      const TaggedValue &mesh_tagged_value = child;
      MeshIndex mesh_index = body_state.createMesh(SceneState::MeshShape());
      SceneState::Mesh &mesh_state = body_state.meshes[mesh_index];
      fillMeshStateFromTaggedValue(mesh_state, mesh_tagged_value);
    }
  }

  return body_index;
}


static BodyIndex
createBodyFromTaggedValueWithoutResolvingConflicts(
  SceneState &result,
  const TaggedValue &tagged_value,
  const Optional<BodyIndex> maybe_parent_index,
  vector<BodyTaggedValue> &body_tagged_values
)
{
  BodyIndex body_index =
    createBodyInSceneState(result, tagged_value, maybe_parent_index);

  createChildBodiesInSceneState(
    result, tagged_value, body_index, body_tagged_values
  );

  addMarkerTaggedValues(tagged_value, body_index, body_tagged_values);

  buildDistanceErrorTaggedValues(
    tagged_value, body_index, body_tagged_values
  );

  return body_index;
}


static void
resolveBodyNameConflictsOnBranch(
  SceneState &scene_state,
  Optional<BodyIndex> maybe_body_index
)
{
  struct Visitor {
    SceneState &scene_state;

    void visitBody(BodyIndex body_index) const
    {
      resolveBodyNameConflicts(body_index, scene_state);
    }

    void visitMarker(MarkerIndex) const
    {
    }
  } visitor = {scene_state};

  forEachBranchIndexInPreOrder(
    maybe_body_index,
    scene_state,
    visitor
  );
}


static void
createFromBodyTaggedValues(
  const vector<BodyTaggedValue> &body_tagged_values,
  SceneState &result,
  MarkerNameMap &marker_name_map,
  Optional<BodyIndex> maybe_body_index
)
{
  createMarkersFromTaggedValues(body_tagged_values, result);

  resolveMarkerNameConflicts(result, marker_name_map);

  createDistanceErrorsFromTaggedValues(
    body_tagged_values, result, marker_name_map
  );

  resolveBodyNameConflictsOnBranch(result, maybe_body_index);
}


BodyIndex
createBodyFromTaggedValue(
  SceneState &result,
  const TaggedValue &tagged_value,
  const Optional<BodyIndex> maybe_parent_index,
  MarkerNameMap &marker_name_map
)
{
  vector<BodyTaggedValue> body_tagged_values;

  BodyIndex body_index =
    createBodyFromTaggedValueWithoutResolvingConflicts(
      result,
      tagged_value,
      maybe_parent_index,
      body_tagged_values
    );

  createFromBodyTaggedValues(
    body_tagged_values, result, marker_name_map, body_index
  );

  return body_index;
}


static void
createChildBodiesInSceneState(
  SceneState &result,
  const TaggedValue &tagged_value,
  const Optional<BodyIndex> maybe_parent_index,
  vector<BodyTaggedValue> &body_tagged_values
)
{
  for (auto &child_tagged_value : tagged_value.children) {
    if (child_tagged_value.tag == "Transform") {
      createBodyFromTaggedValueWithoutResolvingConflicts(
        result,
        child_tagged_value,
        maybe_parent_index,
        body_tagged_values
      );
    }
  }
}


static void
createVariableFromTaggedValue(
  SceneState &result, const TaggedValue &tagged_value
)
{
  Optional<StringValue> maybe_old_name =
    findStringValue(tagged_value, "name");

  VariableIndex variable_index = result.createVariable();

  if (maybe_old_name) {
    result.variables[variable_index].name = *maybe_old_name;
  }

  result.variables[variable_index].value =
    findNumericValue(tagged_value, "value").valueOr(0);
}


static void
createVariablesInSceneState(SceneState &result, const TaggedValue &tagged_value)
{
  for (auto &child_tagged_value : tagged_value.children) {
    if (child_tagged_value.tag == "Variable") {
      createVariableFromTaggedValue(result, child_tagged_value);
    }
  }
}


SceneState makeSceneStateFromTaggedValue(const TaggedValue &tagged_value)
{
  SceneState result;
  Optional<BodyIndex> maybe_parent_index;
  MarkerNameMap marker_name_map;

  vector<BodyTaggedValue> body_tagged_values;

  createChildBodiesInSceneState(
    result, tagged_value, maybe_parent_index,
    body_tagged_values
  );

  addMarkerTaggedValues(tagged_value, maybe_parent_index, body_tagged_values);

  buildDistanceErrorTaggedValues(
    tagged_value, maybe_parent_index, body_tagged_values
  );

  createFromBodyTaggedValues(body_tagged_values, result, marker_name_map, {});

  createVariablesInSceneState(
    result, tagged_value
  );

  return result;
}


static TaggedValue &
create(TaggedValue &parent, const string &tag, bool value)
{
  parent.children.push_back(TaggedValue(tag));
  TaggedValue &result = parent.children.back();

  if (value) {
    result.value = EnumerationValue{"true"};
  }
  else {
    result.value = EnumerationValue{"false"};
  }

  return result;
}


static TaggedValue &create(TaggedValue &parent, const string &tag)
{
  parent.children.push_back(TaggedValue(tag));
  return parent.children.back();
}


static TaggedValue &
  create(TaggedValue &parent, const string &tag, NumericValue value)
{
  parent.children.push_back(TaggedValue(tag));
  TaggedValue &result = parent.children.back();
  result.value = value;
  return result;
}


static TaggedValue &
  create(TaggedValue &parent, const string &tag, StringValue value)
{
  parent.children.push_back(TaggedValue(tag));
  TaggedValue &result = parent.children.back();
  result.value = value;
  return result;
}


static const bool *
maybeSolveFlag(
  const SceneState::XYZSolveFlags *xyz_solve_flags_ptr,
  bool SceneState::XYZSolveFlags::*member_ptr
)
{
  if (!xyz_solve_flags_ptr) {
    return nullptr;
  }

  return &(xyz_solve_flags_ptr->*member_ptr);
}


static const Expression *
maybeExpression(
  const SceneState::XYZExpressions *xyz_expressions_ptr,
  Expression SceneState::XYZExpressions::* member_ptr
)
{
  if (!xyz_expressions_ptr) {
    return nullptr;
  }

  return &(xyz_expressions_ptr->*member_ptr);
}


static void
  create2(
    TaggedValue &parent,
    const string &member_name,
    NumericValue value,
    const bool *solve_flag_ptr,
    const Expression *expression_ptr
  )
{
  TaggedValue &tagged_value = create(parent, member_name, value);

  if (solve_flag_ptr) {
    create(tagged_value, "solve", *solve_flag_ptr);
  }

  if (expression_ptr) {
    if (!expression_ptr->empty()) {
      create(tagged_value, "expression", *expression_ptr);
    }
  }
}


static void
  createXYZChildren(
    TaggedValue &parent,
    const SceneState::XYZ &xyz,
    const SceneState::XYZSolveFlags *xyz_flags_ptr = nullptr,
    const SceneState::XYZExpressions *expressions_ptr = nullptr
  )
{
  using Flags = SceneState::XYZSolveFlags;
  using XYZExpressions = SceneState::XYZExpressions;

  create2(
    parent,
    "x",
    xyz.x,
    maybeSolveFlag(xyz_flags_ptr, &Flags::x),
    maybeExpression(expressions_ptr, &XYZExpressions::x)
  );

  create2(
    parent,
    "y",
    xyz.y,
    maybeSolveFlag(xyz_flags_ptr, &Flags::y),
    maybeExpression(expressions_ptr, &XYZExpressions::y)
  );

  create2(
    parent,
    "z",
    xyz.z,
    maybeSolveFlag(xyz_flags_ptr, &Flags::z),
    maybeExpression(expressions_ptr, &XYZExpressions::z)
  );
}


static TaggedValue &
create(TaggedValue &parent, const string &tag, XYZChannels channel_state)
{
  parent.children.push_back(TaggedValue(tag));
  TaggedValue &result = parent.children.back();

  createXYZChildren(
    result,
    channel_state.values,
    /*solve_flags_ptr*/nullptr,
    &channel_state.expressions
  );

  return result;
}


static TaggedValue &
create(TaggedValue &parent, const string &tag, const SceneState::XYZ &xyz_state)
{
  TaggedValue &tagged_value = create(parent, tag);
  createXYZChildren(tagged_value, xyz_state);
  return tagged_value;
}


static bool
scaleHasDefaultState(
  const TransformState &transform_state,
  const TransformSolveFlags &transform_solve_flags,
  const TransformExpressions &transform_expressions
)
{
  if (transform_state.scale != SceneState::Transform::defaultScale()) {
    return false;
  }

  if (transform_solve_flags.scale != TransformSolveFlags::defaultScale()) {
    return false;
  }

  if (transform_expressions.scale != TransformExpressions::defaultScale()) {
    return false;
  }

  return true;
}


static TaggedValue &
  createTransformInTaggedValue(
    TaggedValue &parent,
    const SceneState::Body::Name &name,
    const TransformState &transform_state,
    const TransformSolveFlags &solve_flags,
    const TransformExpressions &expressions
  )
{
  auto &transform = create(parent, "Transform");
  {
    auto &parent = transform;
    create(parent, "name", name);
    {
      auto &translation = create(parent, "translation");
      auto &parent = translation;

      createXYZChildren(
        parent,
        transform_state.translation,
        &solve_flags.translation,
        &expressions.translation
      );
    }
    {
      auto &rotation = create(parent, "rotation");
      auto &parent = rotation;

      createXYZChildren(
        parent,
        transform_state.rotation,
        &solve_flags.rotation,
        &expressions.rotation
      );
    }

    if (!scaleHasDefaultState(transform_state, solve_flags, expressions)) {
      create2(
        parent,
        "scale",
        transform_state.scale,
        &solve_flags.scale,
        &expressions.scale
      );
    }
  }

  return transform;
}


static TaggedValue &
createBoxInTaggedValue(TaggedValue &parent, const SceneState::Box &box_state)
{
  auto &box_tagged_value = create(parent, "Box");
  create(box_tagged_value, "scale", box_state.scaleChannels());
  create(box_tagged_value, "center", box_state.centerChannels());
  return box_tagged_value;
}


static TaggedValue &
createLineInTaggedValue(TaggedValue &parent, const SceneState::Line &line_state)
{
  auto &box_tagged_value = create(parent, "Line");
  create(box_tagged_value, "start", line_state.start);
  create(box_tagged_value, "end", line_state.end);
  return box_tagged_value;
}


static string str(int index)
{
  ostringstream stream;
  stream << index;
  return stream.str();
}


static TaggedValue &
createMeshInTaggedValue(
  TaggedValue &parent,
  const SceneState::Mesh &mesh_state
)
{
  auto &mesh_tagged_value = create(parent, "Mesh");

  create(mesh_tagged_value, "scale", mesh_state.scale);
  create(mesh_tagged_value, "center", mesh_state.center);

  auto &positions_tagged_value = create(mesh_tagged_value, "positions");

  // Create positions
  for (auto index : indicesOf(mesh_state.shape.positions)) {
    auto &position_tagged_value = create(positions_tagged_value, str(index));
    createXYZChildren(position_tagged_value, mesh_state.shape.positions[index]);
  }

  // Create triangles
  for (auto index : indicesOf(mesh_state.shape.triangles)) {
    auto &triangle_tagged_value = create(mesh_tagged_value, "Triangle");
    NumericValue v1 = mesh_state.shape.triangles[index].v1;
    NumericValue v2 = mesh_state.shape.triangles[index].v2;
    NumericValue v3 = mesh_state.shape.triangles[index].v3;
    create(triangle_tagged_value, "vertex1", v1);
    create(triangle_tagged_value, "vertex2", v2);
    create(triangle_tagged_value, "vertex3", v3);
  }

  return mesh_tagged_value;
}


static void
createMarkerInTaggedValue(
  TaggedValue &parent, const SceneState::Marker &marker_state
)
{
  auto &marker = create(parent, "Marker");
  create(marker, "name", marker_state.name);
  create(marker, "position", marker_state.positionChannels());
}


static void
createVariableInTaggedValue(
  TaggedValue &parent,
  const SceneState::Variable &variable_state
)
{
  auto &variable = create(parent, "Variable");
  create(variable, "name", variable_state.name);
  create(variable, "value", variable_state.value);
}


static void
createMarkerRef(
  TaggedValue &parent, const string &tag, const string &marker_name
)
{
  parent.children.push_back(TaggedValue(tag));
  TaggedValue &result = parent.children.back();
  result.value = EnumerationValue{"MarkerRef"};
  create(result, "marker_name", marker_name);
}


static void
createPointLink(
  const PointLink &point_link,
  const string &tag,
  const SceneState &scene_state,
  TaggedValue &parent
)
{
#if ADD_BODY_MESH_POSITION_TO_POINT_LINK
  if (point_link.maybe_marker) {
    assert(false); // not tested
    MarkerIndex marker_index = point_link.maybe_marker->index;
    auto &marker_name = scene_state.marker(marker_index).name;
    createMarkerRef(parent, tag, marker_name);
  }
  else {
    assert(false); // not implemented
  }
#else
  MarkerIndex marker_index = point_link.marker.index;
  auto &marker_name = scene_state.marker(marker_index).name;
  createMarkerRef(parent, tag, marker_name);
#endif
}


static void
createDistanceErrorInTaggedValue(
  TaggedValue &parent,
  const SceneState::DistanceError &distance_error_state,
  const SceneState &scene_state
)
{
  auto &distance_error = create(parent, "DistanceError");
  {
    auto &parent = distance_error;

    if (distance_error_state.hasStart()) {
      const PointLink &point_link =
        *distance_error_state.optional_start;

      createPointLink(point_link, "start", scene_state, parent);
    }

    if (distance_error_state.hasEnd()) {
      const PointLink &point_link =
        *distance_error_state.optional_end;

      createPointLink(point_link, "end", scene_state, parent);
    }

    create(parent, "desired_distance", distance_error_state.desired_distance);
    create(parent, "weight", distance_error_state.weight);
  }
}


static Optional<BodyIndex>
  maybeAttachedBodyIndex(const SceneState::Marker &marker_state)
{
  return marker_state.maybe_body_index;
}


static Optional<BodyIndex>
  maybeAttachedBodyIndex(const SceneState::DistanceError &distance_error_state)
{
  return distance_error_state.maybe_body_index;
}


static void
  createChildBodiesInTaggedValue(
    TaggedValue &transform,
    const SceneState &scene_state,
    const Optional<BodyIndex> maybe_body_index
  )
{
  for (BodyIndex other_body_index : indicesOf(scene_state.bodies())) {
    const SceneState::Body &other_body_state =
      scene_state.body(other_body_index);

    if (other_body_state.maybe_parent_index == maybe_body_index) {
      createBodyTaggedValue(transform, other_body_index, scene_state);
    }
  }
}


void
  createBodyTaggedValue(
    TaggedValue &parent,
    BodyIndex body_index,
    const SceneState &scene_state
  )
{
  const SceneState::Body &body_state = scene_state.body(body_index);
  const TransformState &transform_state = body_state.transform;

  TaggedValue &transform =
    createTransformInTaggedValue(
      parent,
      body_state.name,
      transform_state,
      body_state.solve_flags,
      body_state.expressions
    );

  for (const SceneState::Box &box_state : body_state.boxes) {
    createBoxInTaggedValue(transform, box_state);
  }

  for (const SceneState::Line &line_state : body_state.lines) {
    createLineInTaggedValue(transform, line_state);
  }

  for (const SceneState::Mesh &mesh_state : body_state.meshes) {
    createMeshInTaggedValue(transform, mesh_state);
  }

  createChildBodiesInTaggedValue(transform, scene_state, body_index);

  for (auto &marker_state : scene_state.markers()) {
    if (maybeAttachedBodyIndex(marker_state) == body_index) {
      createMarkerInTaggedValue(transform, marker_state);
    }
  }

  for (auto &distance_error_state : scene_state.distance_errors) {
    if (maybeAttachedBodyIndex(distance_error_state) == body_index) {
      createDistanceErrorInTaggedValue(
        transform, distance_error_state, scene_state
      );
    }
  }
}


TaggedValue makeTaggedValueForSceneState(const SceneState &scene_state)
{
  TaggedValue result("Scene");

  for (
    const SceneState::Variable &variable_state
    : scene_state.variables
  ) {
    createVariableInTaggedValue(result, variable_state);
  }

  createChildBodiesInTaggedValue(result, scene_state, /*maybe_parent_index*/{});

  for (const SceneState::Marker &marker_state : scene_state.markers()) {
    if (!maybeAttachedBodyIndex(marker_state)) {
      createMarkerInTaggedValue(result, marker_state);
    }
  }

  for (
    const SceneState::DistanceError &distance_error_state
    : scene_state.distance_errors
  ) {
    if (!maybeAttachedBodyIndex(distance_error_state)) {
      createDistanceErrorInTaggedValue(
        result, distance_error_state, scene_state
      );
    }
  }

  return result;
}
