#include "updatescenestatefromscene.hpp"

#include "maketransform.hpp"
#include "indicesof.hpp"
#include "globaltransform.hpp"


static Point
  localTranslation(Scene::TransformHandle transform_id,const Scene &scene)
{
  return scene.translation(transform_id);
}


static void
  updateMarkerPosition(
    SceneState::Marker &state_marker,
    const SceneHandles::Marker &handles_marker,
    const Scene &scene
  )
{
  state_marker.position = localTranslation(handles_marker.handle, scene);
}


static void
  updateStateMarkerPositions(
    SceneState::Markers &state_markers,
    const SceneHandles::Markers &handles_markers,
    const Scene &scene
  )
{
  for (auto i : indicesOf(handles_markers)) {
    updateMarkerPosition(state_markers[i], handles_markers[i], scene);
  }
}


void
  updateSceneStateFromScene(
    SceneState &state,
    const Scene &scene,
    const SceneHandles &scene_handles
  )
{
  updateStateMarkerPositions(state.markers, scene_handles.markers, scene);
  state.box.global = globalTransform(scene, scene_handles.box);
}
