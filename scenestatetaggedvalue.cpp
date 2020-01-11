#include "scenestatetaggedvalue.hpp"

#include "indicesof.hpp"
#include "contains.hpp"
#include "nextunusedname.hpp"

using std::string;
using std::cerr;


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


static void
resolveBodyNameConflicts(BodyIndex body_index, SceneState &scene_state)
{
  std::string &name = scene_state.body(body_index).name;

  if (name[0] == '$') {
    name = nextUnusedName(bodyNames(scene_state), name.substr(1) + "_");
  }
}


static NumericValue
  numericValueOr(
    const TaggedValue &tagged_value,
    const TaggedValue::Tag &child_name,
    NumericValue default_value
  )
{
  return findNumericValue(tagged_value, child_name).valueOr(default_value);
}


static bool
  boolValueOr(
    const TaggedValue &tagged_value,
    const TaggedValue::Tag &child_name,
    bool default_value
  )
{
  return findBoolValue(tagged_value, child_name).valueOr(default_value);
}


static SceneState::XYZ xyzStateFromTaggedValue(const TaggedValue &tagged_value)
{
  NumericValue x = numericValueOr(tagged_value, "x", 0);
  NumericValue y = numericValueOr(tagged_value, "y", 0);
  NumericValue z = numericValueOr(tagged_value, "z", 0);

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

  result.translation = xyzStateFromTaggedValue(*translation_ptr);
  result.rotation = xyzStateFromTaggedValue(*rotation_ptr);

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

  return boolValueOr(*child_ptr, "solve", true);
}


static SceneState::XYZSolveFlags
  makeXYZSolveFlagsFromTaggeDvalue(
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


static SceneState::TransformSolveFlags
  makeSolveFlagsFromTaggedValue(const TaggedValue &tagged_value)
{
  SceneState::TransformSolveFlags result;

  result.translation =
    makeXYZSolveFlagsFromTaggeDvalue(tagged_value, "translation");

  result.rotation =
    makeXYZSolveFlagsFromTaggeDvalue(tagged_value, "rotation");

  return result;
}


static SceneState::XYZ
  xyzValueOr(const TaggedValue &tv, const SceneState::XYZ &default_value)
{
  SceneState::XYZ result;
  result.x = numericValueOr(tv, "x", default_value.x);
  result.y = numericValueOr(tv, "y", default_value.y);
  result.z = numericValueOr(tv, "z", default_value.z);
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
  }

  return marker_index;
}


void
createChildMarkersInSceneState(
  SceneState &scene_state,
  const TaggedValue &tagged_value,
  Optional<BodyIndex> maybe_parent_index,
  MarkerNameMap &marker_name_map
)
{
  for (auto &child_tagged_value : tagged_value.children) {
    if (child_tagged_value.tag == "Marker") {
      createMarkerFromTaggedValueWithoutResolvingConflicts(
        scene_state,
        child_tagged_value,
        maybe_parent_index
      );
    }
  }

  resolveMarkerNameConflicts(scene_state, marker_name_map);
}


static SceneState::XYZ
xyzValueOr(
  const TaggedValue &tagged_value,
  const string &tag,
  const SceneState::XYZ &default_value
)
{
  const TaggedValue *child_ptr = findChild(tagged_value, tag);

  if (child_ptr) {
    return xyzValueOr(*child_ptr, default_value);
  }

  return {default_value.x, default_value.y, default_value.z};
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
    }
    else {
      box_state.scale.x =
        numericValueOr(box_tagged_value, "scale_x", default_scale.x);

      box_state.scale.y =
        numericValueOr(box_tagged_value, "scale_y", default_scale.y);

      box_state.scale.z =
        numericValueOr(box_tagged_value, "scale_z", default_scale.z);
    }
  }

  box_state.center = xyzValueOr(box_tagged_value, "center", {0,0,0});
}


static void
fillLineStateFromTaggedValue(
  SceneState::Line &line_state,
  const TaggedValue &line_tagged_value
)
{
  line_state.start = xyzValueOr(line_tagged_value, "start", {0,0,0});
  line_state.end = xyzValueOr(line_tagged_value, "end", {1,0,0});
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
  createDistanceErrorsInSceneState(
    SceneState &result,
    const TaggedValue &tagged_value,
    Optional<BodyIndex> maybe_body_index,
    const MarkerNameMap &marker_name_map
  )
{
  for (auto &child_tagged_value : tagged_value.children) {
    if (child_tagged_value.tag == "DistanceError") {
      DistanceErrorIndex index = result.createDistanceError(maybe_body_index);

      SceneState::DistanceError &distance_error_state =
        result.distance_errors[index];

      {
        Optional<StringValue> maybe_start_marker_name =
          findStringValue(child_tagged_value, "start");

        if (maybe_start_marker_name) {
          StringValue mapped_marker_name =
            mappedMarkerName(*maybe_start_marker_name, marker_name_map);

          distance_error_state.optional_start_marker_index =
            findMarkerIndex(result, mapped_marker_name);
        }
      }

      {
        Optional<StringValue> maybe_end_marker_name =
          findStringValue(child_tagged_value, "end");

        if (maybe_end_marker_name) {
          StringValue mapped_marker_name =
            mappedMarkerName(*maybe_end_marker_name, marker_name_map);

          distance_error_state.optional_end_marker_index =
            findMarkerIndex(result, mapped_marker_name);
        }
      }

      {
        auto tag = "desired_distance";
        auto optional_value = findNumericValue(child_tagged_value, tag);

        if (optional_value) {
          distance_error_state.desired_distance = *optional_value;
        }
      }

      {
        auto tag = "weight";
        auto optional_value = findNumericValue(child_tagged_value, tag);

        if (optional_value) {
          distance_error_state.weight = *optional_value;
        }
      }
    }
  }
}


static BodyIndex
createBodyFromTaggedValueWithoutResolvingConflicts(
  SceneState &result,
  const TaggedValue &tagged_value,
  const Optional<BodyIndex> maybe_parent_index,
  MarkerNameMap &marker_name_map
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
  setAll(result.body(body_index).solve_flags, true);

  if (maybe_name) {
    result.body(body_index).name = *maybe_name;
  }

  result.body(body_index).transform =
    makeTransformFromTaggedValue(tagged_value);

  result.body(body_index).solve_flags =
    makeSolveFlagsFromTaggedValue(tagged_value);

  for (auto &child : tagged_value.children) {
    if (child.tag == "Box") {
      const TaggedValue &box_tagged_value = child;
      BoxIndex box_index = result.body(body_index).addBox();
      SceneState::Box &box_state = result.body(body_index).boxes[box_index];
      fillBoxStateFromTaggedValue(box_state, box_tagged_value);
    }
  }

  for (auto &child : tagged_value.children) {
    if (child.tag == "Line") {
      const TaggedValue &line_tagged_value = child;
      LineIndex line_index = result.body(body_index).addLine();
      SceneState::Line &line_state = result.body(body_index).lines[line_index];
      fillLineStateFromTaggedValue(line_state, line_tagged_value);
    }
  }

  createChildBodiesInSceneState(
    result, tagged_value, body_index, marker_name_map
  );

  createChildMarkersInSceneState(
    result, tagged_value, body_index, marker_name_map
  );

  createDistanceErrorsInSceneState(
    result, tagged_value, body_index, marker_name_map
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


BodyIndex
createBodyFromTaggedValue(
  SceneState &result,
  const TaggedValue &tagged_value,
  const Optional<BodyIndex> maybe_parent_index,
  MarkerNameMap &marker_name_map
)
{
  BodyIndex body_index =
    createBodyFromTaggedValueWithoutResolvingConflicts(
      result,
      tagged_value,
      maybe_parent_index,
      marker_name_map
    );

  resolveBodyNameConflictsOnBranch(result, body_index);
  return body_index;
}


void
createChildBodiesInSceneState(
  SceneState &result,
  const TaggedValue &tagged_value,
  const Optional<BodyIndex> maybe_parent_index,
  MarkerNameMap &marker_name_map
)
{
  for (auto &child_tagged_value : tagged_value.children) {
    if (child_tagged_value.tag == "Transform") {
      createBodyFromTaggedValue(
        result, child_tagged_value, maybe_parent_index,
        marker_name_map
      );
    }
  }
}


SceneState makeSceneStateFromTaggedValue(const TaggedValue &tagged_value)
{
  SceneState result;
  Optional<BodyIndex> maybe_parent_index;
  MarkerNameMap marker_name_map;

  createChildBodiesInSceneState(
    result, tagged_value, maybe_parent_index, marker_name_map
  );

  createChildMarkersInSceneState(
    result, tagged_value, maybe_parent_index, marker_name_map
  );

  createDistanceErrorsInSceneState(
    result, tagged_value, /*body*/{}, marker_name_map
  );

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


static void
  create2(
    TaggedValue &parent,
    const string &member_name,
    NumericValue value,
    const bool *solve_flag_ptr
  )
{
  TaggedValue &tagged_value = create(parent, member_name, value);

  if (solve_flag_ptr) {
    create(tagged_value, "solve", *solve_flag_ptr);
  }
}


static void
  createXYZChildren(
    TaggedValue &parent,
    const SceneState::XYZ &xyz,
    const SceneState::XYZSolveFlags *xyz_flags_ptr = 0
  )
{
  using Flags = SceneState::XYZSolveFlags;
  create2(parent, "x", xyz.x, maybeSolveFlag(xyz_flags_ptr, &Flags::x));
  create2(parent, "y", xyz.y, maybeSolveFlag(xyz_flags_ptr, &Flags::y));
  create2(parent, "z", xyz.z, maybeSolveFlag(xyz_flags_ptr, &Flags::z));
}


static TaggedValue &
create(TaggedValue &parent, const string &tag, const SceneState::XYZ &xyz_state)
{
  TaggedValue &tagged_value = create(parent, tag);
  createXYZChildren(tagged_value, xyz_state);
  return tagged_value;
}


static TaggedValue &
  createTransform(
    TaggedValue &parent,
    const SceneState::Body::Name &name,
    const TransformState &transform_state,
    const SceneState::TransformSolveFlags &solve_flags
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
        &solve_flags.translation
      );
    }
    {
      auto &rotation = create(parent, "rotation");
      auto &parent = rotation;
      createXYZChildren(parent, transform_state.rotation, &solve_flags.rotation);
    }
  }

  return transform;
}


static TaggedValue &
  createBox(TaggedValue &parent, const SceneState::Box &box_state)
{
  auto &box_tagged_value = create(parent, "Box");
  create(box_tagged_value, "scale", box_state.scale);
  create(box_tagged_value, "center", box_state.center);
  return box_tagged_value;
}


static TaggedValue &
  createLine(TaggedValue &parent, const SceneState::Line &line_state)
{
  auto &box_tagged_value = create(parent, "Line");
  create(box_tagged_value, "start", line_state.start);
  create(box_tagged_value, "end", line_state.end);
  return box_tagged_value;
}


static void
createMarker(TaggedValue &parent, const SceneState::Marker &marker_state)
{
  auto &marker = create(parent, "Marker");
  create(marker, "name", marker_state.name);
  create(marker, "position", marker_state.position);
}


static void
  createDistanceError(
    TaggedValue &parent,
    const SceneState::DistanceError &distance_error_state,
    const SceneState &scene_state
  )
{
  auto &distance_error = create(parent, "DistanceError");
  {
    auto &parent = distance_error;

    if (distance_error_state.optional_start_marker_index) {
      MarkerIndex start_marker_index =
        *distance_error_state.optional_start_marker_index;

      auto &start_marker_name = scene_state.marker(start_marker_index).name;
      create(parent, "start", start_marker_name);
    }

    if (distance_error_state.optional_end_marker_index) {
      MarkerIndex end_marker_index =
        *distance_error_state.optional_end_marker_index;

      auto &end_marker_name = scene_state.marker(end_marker_index).name;
      create(parent, "end", end_marker_name);
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
    createTransform(
      parent,
      body_state.name,
      transform_state,
      body_state.solve_flags
    );

  for (const SceneState::Box &box_state : body_state.boxes) {
    createBox(transform, box_state);
  }

  for (const SceneState::Line &line_state : body_state.lines) {
    createLine(transform, line_state);
  }

  createChildBodiesInTaggedValue(transform, scene_state, body_index);

  for (auto &marker_state : scene_state.markers()) {
    if (maybeAttachedBodyIndex(marker_state) == body_index) {
      createMarker(transform, marker_state);
    }
  }

  for (auto &distance_error_state : scene_state.distance_errors) {
    if (maybeAttachedBodyIndex(distance_error_state) == body_index) {
      createDistanceError(transform, distance_error_state, scene_state);
    }
  }
}


TaggedValue makeTaggedValueForSceneState(const SceneState &scene_state)
{
  TaggedValue result("Scene");
  createChildBodiesInTaggedValue(result, scene_state, /*maybe_parent_index*/{});

  for (const SceneState::Marker &marker_state : scene_state.markers()) {
    if (!maybeAttachedBodyIndex(marker_state)) {
      createMarker(result, marker_state);
    }
  }

  for (
    const SceneState::DistanceError &distance_error_state
    : scene_state.distance_errors
  ) {
    if (!maybeAttachedBodyIndex(distance_error_state)) {
      createDistanceError(result, distance_error_state, scene_state);
    }
  }

  return result;
}
