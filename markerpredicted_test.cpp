#include "markerpredicted.hpp"


static BodyIndex
  createBodyIn(SceneState &scene_state, Optional<BodyIndex> maybe_parent_index)
{
  return scene_state.createBody(maybe_parent_index);
}


static BodyIndex createGlobalBodyIn(SceneState &scene_state)
{
  return createBodyIn(scene_state, /*maybe_parent_index*/{});
}


static void testWithGlobalMarkerAtOrigin()
{
  SceneState scene_state;
  MarkerIndex marker_index = scene_state.createMarker("marker");
  Point p = markerPredicted(scene_state, marker_index);
  assert(p == Point(0,0,0));
}


static void testWithHierarchy()
{
  SceneState scene_state;
  BodyIndex body1_index = createGlobalBodyIn(scene_state);
  scene_state.body(body1_index).transform.translation.y = 1;

  BodyIndex body2_index =
    createBodyIn(scene_state, /*parent_index*/body1_index);

  scene_state.body(body2_index).transform.translation.y = 2;

  MarkerIndex marker_index = scene_state.createMarker("marker");
  scene_state.marker(marker_index).maybe_body_index = body2_index;

  Point p = markerPredicted(scene_state, marker_index);

  assert(p == Point(0,3,0));
}


int main()
{
  testWithGlobalMarkerAtOrigin();
  testWithHierarchy();
}
