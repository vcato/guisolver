#include "scenestate.hpp"


int main()
{
  SceneState scene_state;
  BodyIndex body1_index = scene_state.createBody();
  BodyIndex body2_index = scene_state.createBody();
  MarkerIndex marker_index = scene_state.createMarker(body2_index);
  scene_state.removeBody(body1_index);
  assert(scene_state.marker(marker_index).maybe_body_index == 0);
}
