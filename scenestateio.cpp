#include "scenestateio.hpp"

#include "taggedvalue.hpp"
#include "taggedvalueio.hpp"
#include "vec3.hpp"
#include "rotationvector.hpp"
#include "maketransform.hpp"
#include "indicesof.hpp"
#include "transformstate.hpp"
#include "positionstate.hpp"

using std::ostream;
using std::cerr;
using std::string;


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


static void createXYZChildren(TaggedValue &parent, const Vec3 &value)
{
  create(parent, "x", value.x);
  create(parent, "y", value.y);
  create(parent, "z", value.z);
}


static TaggedValue &
  createTransform(
    TaggedValue &result,
    const TransformState &transform_state
  )
{
  auto &transform = create(result, "Transform");
  auto &parent = transform;
  {
    auto &translation = create(parent, "translation");
    auto &parent = translation;
    createXYZChildren(parent, translationValues(transform_state));
  }
  {
    auto &rotation = create(parent, "rotation");
    auto &parent = rotation;
    Vec3 r_deg = rotationValuesDeg(transform_state);
    createXYZChildren(parent, r_deg);
  }

  return transform;
}


static TaggedValue &
  createBox(TaggedValue &parent, const SceneState::Body &box_state)
{
  auto &box = create(parent, "Box");
  {
    auto &parent = box;
    create(parent, "scale_x", box_state.scale.x);
    create(parent, "scale_y", box_state.scale.y);
    create(parent, "scale_z", box_state.scale.z);
  }

  return box;
}


static void
createMarker(TaggedValue &parent, const SceneState::Marker &marker_state)
{
  auto &marker = create(parent, "Marker");
  {
    auto &parent = marker;
    create(parent, "name", marker_state.name);
    TaggedValue &position = create(parent, "position");
    createXYZChildren(position, vec3(marker_state.position));
  }
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
  createBody(
    TaggedValue &parent,
    BodyIndex body_index,
    const SceneState &scene_state
  );


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
      createBody(transform, other_body_index, scene_state);
    }
  }
}


static void
  createBody(
    TaggedValue &parent,
    BodyIndex body_index,
    const SceneState &scene_state
  )
{
  const SceneState::Body &body_state = scene_state.body(body_index);
  const TransformState &transform_state = body_state.global;
  TaggedValue &transform = createTransform(parent, transform_state);
  createBox(transform, body_state);
  createChildBodies(transform, scene_state, body_index);

  for (const SceneState::Marker &marker_state : scene_state.markers()) {
    if (maybeAttachedBodyIndex(marker_state) == body_index) {
      createMarker(transform, marker_state);
    }
  }
}


static TaggedValue makeTaggedValue(const SceneState &scene_state)
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


static const TaggedValue *
  findChild(const TaggedValue &tagged_value, const TaggedValue::Tag &tag)
{
  for (auto &child : tagged_value.children) {
    if (child.tag == tag) {
      return &child;
    }
  }

  return nullptr;
}


static Optional<NumericValue>
  findNumericValue(
    const TaggedValue &tagged_value,
    const TaggedValue::Tag &child_name
  )
{
  const TaggedValue *x_ptr = findChild(tagged_value, child_name);

  if (!x_ptr) {
    return {};
  }

  if (!x_ptr->value.isNumeric()) {
    return {};
  }

  return x_ptr->value.asNumeric();
}


static Optional<StringValue>
  findStringValue(
    const TaggedValue &tagged_value,
    const TaggedValue::Tag &child_name
  )
{
  const TaggedValue *x_ptr = findChild(tagged_value, child_name);

  if (!x_ptr) {
    return {};
  }

  if (!x_ptr->value.isString()) {
    return {};
  }

  return x_ptr->value.asString();
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


static Vec3 makeVec3(const TaggedValue &tagged_value)
{
  NumericValue x = numericValueOr(tagged_value, "x", 0);
  NumericValue y = numericValueOr(tagged_value, "y", 0);
  NumericValue z = numericValueOr(tagged_value, "z", 0);

  return {x,y,z};
}


static TransformState makeTransform(const TaggedValue &tagged_value)
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

  Vec3 translation = makeVec3(*translation_ptr);
  Vec3 rotation = makeVec3(*rotation_ptr);

  setTranslationValues(result, translation);
  setRotationValuesDeg(result, rotation);

  return result;
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
      makeMarkerPosition(eigenVector3f(makeVec3(*position_ptr)));
  }
}


static void
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


static void
  extractDistanceErrors(SceneState &result, const TaggedValue &tagged_value)
{
  for (auto &child_tagged_value : tagged_value.children) {
    if (child_tagged_value.tag == "DistanceError") {
      SceneState::DistanceError &distance_error_state =
        result.addDistanceError();

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


static void
  extractBodies(
    SceneState &result,
    const TaggedValue &tagged_value,
    const Optional<BodyIndex> maybe_parent_index
  );


static BodyIndex
  createBodyFromTaggedValue(
    SceneState &result,
    const TaggedValue &tagged_value,
    const Optional<BodyIndex> maybe_parent_index
  )
{
  BodyIndex body_index = createBodyInState(result, maybe_parent_index);
  result.body(body_index).global = makeTransform(tagged_value);
  extractBodies(result, tagged_value, body_index);
  return body_index;
}


static void
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


static SceneState makeSceneState(const TaggedValue &tagged_value)
{
  SceneState result;
  Optional<BodyIndex> maybe_parent_index;
  extractBodies(result, tagged_value, maybe_parent_index);
  extractMarkers(result, tagged_value, maybe_parent_index);
  extractDistanceErrors(result, tagged_value);
  return result;
}


void printSceneStateOn(ostream &stream, const SceneState &state)
{
  printTaggedValueOn(stream, makeTaggedValue(state), /*indent*/0);
}


Expected<SceneState> scanSceneStateFrom(std::istream &stream)
{
  Expected<TaggedValue> expected_tagged_value = scanTaggedValueFrom(stream);

  if (expected_tagged_value.isError()) {
    return expected_tagged_value.asError();
  }

  return makeSceneState(expected_tagged_value.asValue());
}
