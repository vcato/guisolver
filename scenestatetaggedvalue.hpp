#include "scenestate.hpp"
#include "taggedvalue.hpp"
#include "markernamemap.hpp"


extern BodyIndex
  createBodyFromTaggedValue(
    SceneState &result,
    const TaggedValue &tagged_value,
    const Optional<BodyIndex> maybe_parent_index,
    MarkerNameMap &marker_name_map
  );

extern void
  createBodyTaggedValue(
    TaggedValue &parent,
    BodyIndex body_index,
    const SceneState &scene_state
  );

extern SceneState
  makeSceneStateFromTaggedValue(const TaggedValue &tagged_value);

extern TaggedValue makeTaggedValueForSceneState(const SceneState &scene_state);
