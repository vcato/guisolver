#include "scenestatetaggedvalue.hpp"

#include "vec3.hpp"
#include "transformstate.hpp"
#include "positionstate.hpp"
#include "indicesof.hpp"

using std::string;


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


static Vec3 makeVec3FromTaggedValue(const TaggedValue &tagged_value)
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

  Vec3 translation = makeVec3FromTaggedValue(*translation_ptr);
  Vec3 rotation = makeVec3FromTaggedValue(*rotation_ptr);

  setTranslationValues(result, translation);
  setRotationValuesDeg(result, rotation);

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
  xyzValueOr(const TaggedValue &tv, const Vec3 &default_value)
{
  SceneState::XYZ result;
  result.x = numericValueOr(tv, "x", default_value.x);
  result.y = numericValueOr(tv, "y", default_value.y);
  result.z = numericValueOr(tv, "z", default_value.z);
  return result;
}


BodyIndex
  createBodyFromTaggedValue(
    SceneState &result,
    const TaggedValue &tagged_value,
    const Optional<BodyIndex> maybe_parent_index
  )
{
  BodyIndex body_index = createBodyInState(result, maybe_parent_index);

  result.body(body_index).transform =
    makeTransformFromTaggedValue(tagged_value);

  result.body(body_index).solve_flags =
    makeSolveFlagsFromTaggedValue(tagged_value);

  const TaggedValue *box_ptr = findChild(tagged_value, "Box");

  if (box_ptr) {
    const TaggedValue &box = *box_ptr;
    {
      const TaggedValue *scale_ptr = findChild(box, "scale");
      const Vec3 default_scale = {1,1,1};

      if (scale_ptr) {
        result.body(body_index).geometry.scale =
          xyzValueOr(*scale_ptr, default_scale);
      }
      else {
        result.body(body_index).geometry.scale.x =
          numericValueOr(box, "scale_x", default_scale.x);

        result.body(body_index).geometry.scale.y =
          numericValueOr(box, "scale_y", default_scale.y);

        result.body(body_index).geometry.scale.z =
          numericValueOr(box, "scale_z", default_scale.z);
      }
    }
    {
      const TaggedValue *center_ptr = findChild(box, "center");

      if (center_ptr) {
        result.body(body_index).geometry.center =
          xyzValueOr(*center_ptr, {0,0,0});
      }
    }
  }

  extractBodies(result, tagged_value, body_index);
  return body_index;
}


static void
  createMarkerFromTaggedValue(
    SceneState &scene_state,
    const TaggedValue &tagged_value,
    Optional<BodyIndex> maybe_parent_index
  )
{
  Optional<StringValue> maybe_name =
    findStringValue(tagged_value, "name");

  Optional<MarkerIndex> maybe_marker_index;

  if (maybe_name) {
    maybe_marker_index = scene_state.createMarker(*maybe_name);
  }
  else {
    assert(false);
  }

  assert(maybe_marker_index);
  MarkerIndex marker_index = *maybe_marker_index;
  scene_state.marker(marker_index).maybe_body_index = maybe_parent_index;
  const TaggedValue *position_ptr = findChild(tagged_value, "position");

  if (position_ptr) {
    scene_state.marker(marker_index).position =
      makeMarkerPosition(eigenVector3f(makeVec3FromTaggedValue(*position_ptr)));
  }
}


void
  extractMarkers(
    SceneState &scene_state,
    const TaggedValue &tagged_value,
    Optional<BodyIndex> maybe_parent_index
  )
{
  for (auto &child_tagged_value : tagged_value.children) {
    if (child_tagged_value.tag == "Marker") {
      createMarkerFromTaggedValue(
        scene_state,
        child_tagged_value,
        maybe_parent_index
      );
    }
  }
}


void
  extractBodies(
    SceneState &result,
    const TaggedValue &tagged_value,
    const Optional<BodyIndex> maybe_parent_index
  )
{
  for (auto &child_tagged_value : tagged_value.children) {
    if (child_tagged_value.tag == "Transform") {
      BodyIndex body_index =
        createBodyFromTaggedValue(
          result,
          child_tagged_value,
          maybe_parent_index
        );

      extractMarkers(result, child_tagged_value, body_index);
    }
  }
}


static Optional<MarkerIndex>
  findMarkerIndex(
    const SceneState &scene_state,
    const SceneState::Marker::Name &name
  )
{
  for (auto i : indicesOf(scene_state.markers())) {
    if (scene_state.marker(i).name == name) {
      return i;
    }
  }

  return {};
}


void
  extractDistanceErrors(SceneState &result, const TaggedValue &tagged_value)
{
  for (auto &child_tagged_value : tagged_value.children) {
    if (child_tagged_value.tag == "DistanceError") {
      DistanceErrorIndex index = result.createDistanceError();

      SceneState::DistanceError &distance_error_state =
        result.distance_errors[index];

      {
        Optional<StringValue> maybe_start_marker_name =
          findStringValue(child_tagged_value, "start");

        if (maybe_start_marker_name) {
          distance_error_state.optional_start_marker_index =
            findMarkerIndex(result, *maybe_start_marker_name);
        }
      }

      {
        Optional<StringValue> maybe_end_marker_name =
          findStringValue(child_tagged_value, "end");

        if (maybe_end_marker_name) {
          distance_error_state.optional_end_marker_index =
            findMarkerIndex(result, *maybe_end_marker_name);
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


SceneState makeSceneStateFromTaggedValue(const TaggedValue &tagged_value)
{
  SceneState result;
  Optional<BodyIndex> maybe_parent_index;
  extractBodies(result, tagged_value, maybe_parent_index);
  extractMarkers(result, tagged_value, maybe_parent_index);
  extractDistanceErrors(result, tagged_value);
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
    if (!*solve_flag_ptr) {
      create(tagged_value, "solve", *solve_flag_ptr);
    }
  }
}


static void
  createXYZChildren(
    TaggedValue &parent,
    const Vec3 &value,
    const SceneState::XYZSolveFlags *xyz_flags_ptr = 0
  )
{
  using Flags = SceneState::XYZSolveFlags;
  create2(parent, "x", value.x, maybeSolveFlag(xyz_flags_ptr, &Flags::x));
  create2(parent, "y", value.y, maybeSolveFlag(xyz_flags_ptr, &Flags::y));
  create2(parent, "z", value.z, maybeSolveFlag(xyz_flags_ptr, &Flags::z));
}


static void
  createXYZChildren(
    TaggedValue &parent,
    const SceneState::XYZ &xyz
  )
{
  createXYZChildren(parent, vec3(xyz));
}


static TaggedValue &
create(TaggedValue &parent, const string &tag, const SceneState::XYZ &xyz)
{
  TaggedValue &tagged_value = create(parent, tag);
  createXYZChildren(tagged_value, xyz);
  return tagged_value;
}


static TaggedValue &
  createTransform(
    TaggedValue &parent,
    const TransformState &transform_state,
    const SceneState::TransformSolveFlags &solve_flags
  )
{
  auto &transform = create(parent, "Transform");
  {
    auto &parent = transform;
    {
      auto &translation = create(parent, "translation");
      auto &parent = translation;

      createXYZChildren(
        parent,
        translationValues(transform_state),
        &solve_flags.translation
      );
    }
    {
      auto &rotation = create(parent, "rotation");
      auto &parent = rotation;
      Vec3 r_deg = rotationValuesDeg(transform_state);
      createXYZChildren(parent, r_deg, &solve_flags.rotation);
    }
  }

  return transform;
}


static TaggedValue &
  createBox(TaggedValue &parent, const SceneState::Body &box_state)
{
  auto &box = create(parent, "Box");
  create(box, "scale", box_state.geometry.scale);
  create(box, "center", box_state.geometry.center);
  return box;
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
    const SceneState &scene_state,
    const SceneState::DistanceError &distance_error_state
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


static void
  createChildBodies(
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
    createTransform(parent, transform_state, body_state.solve_flags);

  createBox(transform, body_state);
  createChildBodies(transform, scene_state, body_index);

  for (const SceneState::Marker &marker_state : scene_state.markers()) {
    if (maybeAttachedBodyIndex(marker_state) == body_index) {
      createMarker(transform, marker_state);
    }
  }
}


TaggedValue makeTaggedValueForSceneState(const SceneState &scene_state)
{
  TaggedValue result("Scene");
  createChildBodies(result, scene_state, /*maybe_parent_index*/{});

  for (const SceneState::Marker &marker_state : scene_state.markers()) {
    if (!maybeAttachedBodyIndex(marker_state)) {
      createMarker(result, marker_state);
    }
  }

  for (
    const SceneState::DistanceError &distance_error_state
    : scene_state.distance_errors
  ) {
    createDistanceError(result, scene_state, distance_error_state);
  }

  return result;
}
