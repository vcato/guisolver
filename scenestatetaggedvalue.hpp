#include "scenestate.hpp"
#include "taggedvalue.hpp"


extern void
  createChildBodiesInSceneState(
    SceneState &result,
    const TaggedValue &tagged_value,
    const Optional<BodyIndex> maybe_parent_index
  );

extern BodyIndex
  createBodyFromTaggedValue(
    SceneState &result,
    const TaggedValue &tagged_value,
    const Optional<BodyIndex> maybe_parent_index
  );

extern void
  createChildMarkersInSceneState(
    SceneState &scene_state,
    const TaggedValue &tagged_value,
    Optional<BodyIndex> maybe_parent_index
  );

extern void
  createDistanceErrorsInSceneState(
    SceneState &result,
    const TaggedValue &tagged_value
  );

extern SceneState
  makeSceneStateFromTaggedValue(const TaggedValue &tagged_value);

extern TaggedValue makeTaggedValueForSceneState(const SceneState &scene_state);

extern void
  createBodyTaggedValue(
    TaggedValue &parent,
    BodyIndex body_index,
    const SceneState &scene_state
  );
