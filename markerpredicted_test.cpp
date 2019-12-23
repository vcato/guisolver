#include "markerpredicted.hpp"


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
  BodyIndex body1_index = scene_state.createBody(/*maybe_parent_index*/{});

  scene_state.body(body1_index).transform.translation.y = 1;

  BodyIndex body2_index =
    scene_state.createBody(/*maybe_parent_index*/body1_index);

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
