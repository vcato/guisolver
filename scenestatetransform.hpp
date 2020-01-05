#include "bodyindex.hpp"
#include "scenestate.hpp"


extern void
changeBodyTransformToPreserveGlobal(
  BodyIndex new_body_index,
  SceneState &scene_state,
  Optional<BodyIndex> maybe_old_parent_body_index,
  Optional<BodyIndex> maybe_new_parent_body_index
);


extern void
changeMarkerPositionToPreserveGlobal(
  MarkerIndex marker_index,
  SceneState &scene_state,
  Optional<BodyIndex> maybe_old_parent_body_index,
  Optional<BodyIndex> maybe_new_parent_body_index
);
