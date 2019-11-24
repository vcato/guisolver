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


void printSceneStateOn(ostream &stream, const SceneState &state)
{
  printTaggedValueOn(stream, makeTaggedValue(state), /*indent*/0);
}
