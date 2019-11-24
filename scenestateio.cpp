#include "scenestateio.hpp"

#include "taggedvalue.hpp"
#include "taggedvalueio.hpp"
#include "vec3.hpp"
#include "rotationvector.hpp"

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
  createTransform(TaggedValue &result, const Transform &transform_state)
{
  auto &transform = create(result, "Transform");
  auto &parent = transform;
  {
    auto &translation = create(parent, "translation");
    auto &parent = translation;
    createXYZChildren(parent, vec3(transform_state.translation()));
  }
  {
    auto &rotation = create(parent, "rotation");
    auto &parent = rotation;
    auto &value = transform_state.rotation();
    Vec3 r_rad = rotationVector(value);
    Vec3 r_deg = r_rad * (180/M_PI);
    createXYZChildren(parent, r_deg);
  }

  return transform;
}


static TaggedValue &
  createBox(TaggedValue &parent, const SceneState::Box &box_state)
{
  auto &box = create(parent, "Box");
  {
    auto &parent = box;
    create(parent, "scale_x", box_state.scale_x);
    create(parent, "scale_y", box_state.scale_y);
    create(parent, "scale_z", box_state.scale_z);
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


static TaggedValue makeTaggedValue(const SceneState &scene_state)
{
  TaggedValue result("Scene");
  {
    auto &parent = result;
    const Transform &transform_state = scene_state.box.global;
    TaggedValue &transform = createTransform(parent, transform_state);
    {
      auto &parent = transform;
      createBox(parent, scene_state.box);

      for (const SceneState::Marker &marker_state : scene_state.markers()) {
        if (marker_state.is_local) {
          createMarker(parent, marker_state);
        }
      }
    }

    for (const SceneState::Marker &marker_state : scene_state.markers()) {
      if (!marker_state.is_local) {
        createMarker(parent, marker_state);
      }
    }
  }
  return result;
}


#if ADD_SCAN_SCENE_STATE
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
#endif


#if ADD_SCAN_SCENE_STATE
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
#endif


#if ADD_SCAN_SCENE_STATE
static NumericValue
  numericValueOr(
    const TaggedValue &tagged_value,
    const TaggedValue::Tag &child_name,
    NumericValue default_value
  )
{
  return findNumericValue(tagged_value, child_name).valueOr(default_value);
}
#endif


#if ADD_SCAN_SCENE_STATE
static Vec3 makeVec3(const TaggedValue &tagged_value)
{
  NumericValue x = numericValueOr(tagged_value, "x", 0);
  NumericValue y = numericValueOr(tagged_value, "y", 0);
  NumericValue z = numericValueOr(tagged_value, "z", 0);

  return {x,y,z};
}
#endif


#if ADD_SCAN_SCENE_STATE
static Transform makeTransform(const TaggedValue &tagged_value)
{
  Transform result;

  const TaggedValue *translation_ptr = findChild(tagged_value, "translation");

  if (!translation_ptr) {
    assert(false);
  }

  const TaggedValue *rotation_ptr = findChild(tagged_value, "rotation");

  if (!rotation_ptr) {
    assert(false);
  }

  // Vec3 translation = makeVec3(*translation_ptr);
  // Vec3 rotation = makeVec3(*rotation_ptr);
  // result.setRotation(makeRotation(rotation));
  printTaggedValueOn(cerr, tagged_value);
  cerr << "makeTransform() Not implemented\n";
  assert(false);

  return result;
}
#endif


#if ADD_SCAN_SCENE_STATE
static SceneState makeSceneState(const TaggedValue &tagged_value)
{
  const TaggedValue *transform_ptr = findChild(tagged_value, "Transform");

  if (!transform_ptr) {
    assert(false);
  }

  SceneState result;
  result.box.global = makeTransform(*transform_ptr);
  return result;
}
#endif



void printSceneStateOn(ostream &stream, const SceneState &state)
{
  printTaggedValueOn(stream, makeTaggedValue(state), /*indent*/0);
}


#if ADD_SCAN_SCENE_STATE
Expected<SceneState> scanSceneStateFrom(std::istream &stream)
{
  Expected<TaggedValue> expected_tagged_value = scanTaggedValueFrom(stream);

  if (expected_tagged_value.isError()) {
    return expected_tagged_value.asError();
  }

  return makeSceneState(expected_tagged_value.asValue());
}
#endif
