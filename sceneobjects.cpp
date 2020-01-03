#include "sceneobjects.hpp"

#include <iostream>
#include "settransform.hpp"
#include "maketransform.hpp"
#include "indicesof.hpp"
#include "removeindexfrom.hpp"
#include "transformstate.hpp"
#include "positionstate.hpp"
#include "globaltransform.hpp"

using std::cerr;
using TransformHandle = Scene::TransformHandle;
using BoxAndTransformHandle = Scene::BoxAndTransformHandle;
using LineAndTransformHandle = Scene::LineAndTransformHandle;

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
  state_marker.position =
    makeMarkerPosition(localTranslation(handles_marker.handle.transform_handle, scene));
}


static void
updateStateMarkerPositions(
  SceneState &scene_state,
  const SceneHandles &scene_handles,
  const Scene &scene
)
{
  for (auto i : indicesOf(scene_handles.markers)) {
    updateMarkerPosition(
      scene_state.marker(i), scene_handles.marker(i), scene
    );
  }
}


static Transform
localTransform(const Scene &scene, Scene::TransformHandle transform_id)
{
  Point translation = scene.translation(transform_id);
  CoordinateAxes coordinate_axes = scene.coordinateAxes(transform_id);
  return makeTransform(coordinate_axes, translation);
}


void
updateSceneStateFromSceneObjects(
  SceneState &state,
  const Scene &scene,
  const SceneHandles &scene_handles
)
{
  updateStateMarkerPositions(state, scene_handles, scene);

  for (auto i : indicesOf(state.bodies())) {
    const BoxAndTransformHandle body_handle = scene_handles.body(i);
    SceneState::Body &body_state = state.body(i);
    body_state.transform = transformState(localTransform(scene, body_handle.transform_handle));
    body_state.box().scale = xyzState(scene.geometryScale(body_handle));
    body_state.box().center = xyzState(scene.geometryCenter(body_handle));
  }
}


static SceneHandles::Marker
  createSceneLocal(
    Scene &scene,
    BoxAndTransformHandle &parent,
    Scene::Point position
  )
{
  auto point = scene.createSphereAndTransform(parent.transform_handle);
  scene.setGeometryScale(point, {0.1, 0.1, 0.1});
  scene.setGeometryColor(point, 0, 0, 1);
  scene.setTranslation(point.transform_handle, position);
  SceneHandles::Marker marker_handles = SceneHandles::Marker{point};
  return marker_handles;
}


static SceneHandles::Marker
  createSceneGlobal(
    Scene &scene,
    Scene::Point position
  )
{
  auto point = scene.createSphereAndTransform();
  scene.setGeometryScale(point, {0.1, 0.1, 0.1});
  scene.setGeometryColor(point, 0, 1, 0);
  scene.setTranslation(point.transform_handle, position);
  SceneHandles::Marker marker_handles = SceneHandles::Marker{point};
  return marker_handles;
}


static SceneHandles::Marker
  createSceneMarker(
    Scene &scene,
    const SceneState::Marker &state_marker,
    const SceneHandles &scene_handles
  )
{
  Point position = makePoint(state_marker.position);

  Optional<BodyIndex> maybe_body_index = state_marker.maybe_body_index;

  if (maybe_body_index) {
    BodyIndex parent_body_index = *maybe_body_index;
    BoxAndTransformHandle box_handle = scene_handles.body(parent_body_index);
    return createSceneLocal(scene, box_handle, position);
  }
  else {
    return createSceneGlobal(scene, position);
  }
}


static SceneHandles::DistanceError createDistanceError(Scene &scene)
{
  LineAndTransformHandle line = scene.createLineAndTransform(scene.top());
  scene.setGeometryColor(line, 1,0,0);
  return {line};
}


static void
updateDistanceErrorInScene(
  Scene &scene,
  const SceneState &scene_state,
  const SceneHandles &scene_handles,
  DistanceErrorIndex distance_error_index
)
{
  const SceneHandles::DistanceError &distance_error_handles =
    scene_handles.distance_errors[distance_error_index];

  const SceneState::DistanceError &distance_error_state =
    scene_state.distance_errors[distance_error_index];

  bool have_both_markers =
    distance_error_state.optional_start_marker_index &&
    distance_error_state.optional_end_marker_index;

  Scene::Point start = {0,0,0};
  Scene::Point end = {0,0,0};

  if (have_both_markers) {
    MarkerIndex start_marker_index =
      *distance_error_state.optional_start_marker_index;

    MarkerIndex end_marker_index =
      *distance_error_state.optional_end_marker_index;

    start = markerPredicted(scene_state, start_marker_index);
    end = markerPredicted(scene_state, end_marker_index);
  }

  scene.setStartPoint(distance_error_handles.line, start);
  scene.setEndPoint(distance_error_handles.line, end);
}


static void
createDistanceErrorInScene1(
  Scene &scene,
  SceneHandles &scene_handles,
  DistanceErrorIndex index
)
{
  vector<SceneHandles::DistanceError> &distance_error_handles =
    scene_handles.distance_errors;

  if (index != DistanceErrorIndex(distance_error_handles.size())) {
    assert(false); // not implemented
  }
  else {
    distance_error_handles.push_back(createDistanceError(scene));
  }
}


void
createDistanceErrorInScene(
  Scene &scene,
  SceneHandles &scene_handles,
  const SceneState &scene_state,
  DistanceErrorIndex index
)
{
  createDistanceErrorInScene1(scene, scene_handles, index);
  updateDistanceErrorInScene(scene, scene_state, scene_handles, index);
}


void
  removeDistanceErrorFromScene(
    Scene &scene,
    SceneHandles::DistanceErrors &distance_errors,
    int index
  )
{
  scene.destroyTransformAndGeometry(distance_errors[index].line);
  removeIndexFrom(distance_errors, index);
}


void
removeMarkerObjectFromScene(
  MarkerIndex index,
  Scene &scene,
  SceneHandles &scene_handles
)
{
  scene.destroyTransformAndGeometry(scene_handles.marker(index).handle);
  scene_handles.markers[index].reset();
}


void
  removeMarkerFromScene(
    Scene &scene,
    SceneHandles &scene_handles,
    MarkerIndex index
  )
{
  removeMarkerObjectFromScene(index, scene, scene_handles);
  removeIndexFrom(scene_handles.markers, index);
}


void
createMarkerObjectInScene(
  MarkerIndex marker_index,
  Scene &scene,
  SceneHandles &scene_handles,
  const SceneState &state
)
{
  const SceneState::Marker &state_marker = state.marker(marker_index);

  scene_handles.markers[marker_index] =
    createSceneMarker(scene,state_marker,scene_handles);
}


void
createMarkerInScene(
  Scene &scene,
  SceneHandles &scene_handles,
  const SceneState &state,
  MarkerIndex marker_index
)
{
  scene_handles.markers.emplace_back();
  createMarkerObjectInScene(marker_index, scene, scene_handles, state);
}


static BoxAndTransformHandle
createBodyTransform(
  const SceneState::Body &body_state,
  Scene &scene,
  const SceneHandles &scene_handles
)
{
  if (body_state.maybe_parent_index) {
    const BodyIndex parent_body_index = *body_state.maybe_parent_index;

    BoxAndTransformHandle parent_transform_handle =
      scene_handles.body(parent_body_index);

    return scene.createBoxAndTransform(parent_transform_handle.transform_handle);
  }
  else {
    return scene.createBoxAndTransform();
  }
}


static void
  updateBodyInScene(
    Scene &scene,
    const BoxAndTransformHandle &body_transform_handle,
    const SceneState::Body &body_state
  )
{
  setTransform(
    body_transform_handle.transform_handle,
    makeTransformFromState(body_state.transform),
    scene
  );

  scene.setGeometryScale(
    body_transform_handle,
    vec3(body_state.box().scale)
  );

  scene.setGeometryCenter(
    body_transform_handle,
    point(body_state.box().center)
  );
}


static void
createBodyObjectInScene(
  BodyIndex body_index,
  Scene &scene,
  SceneHandles &scene_handles,
  const SceneState &state
)
{
  const SceneState::Body &body_state = state.body(body_index);

  BoxAndTransformHandle transform_handle =
    createBodyTransform(body_state, scene, scene_handles);

  updateBodyInScene(scene, transform_handle, body_state);
  assert(!scene_handles.bodies[body_index].hasValue());
  scene_handles.bodies[body_index] = transform_handle;
}


void
createBodyInScene(
  Scene &scene,
  SceneHandles &scene_handles,
  const SceneState &state,
  BodyIndex body_index
)
{
  if (body_index > BodyIndex(scene_handles.bodies.size())) {
    assert(false); // not implemented
  }

  assert(BodyIndex(scene_handles.bodies.size()) == body_index);
  scene_handles.bodies.emplace_back();

  createBodyObjectInScene(
    body_index,
    scene,
    scene_handles,
    state
  );

  for (auto marker_index : indicesOfMarkersOnBody(body_index, state)) {
    createMarkerInScene(scene, scene_handles, state, marker_index);
  }

  for (auto child_body_index : indicesOfChildBodies(body_index, state)) {
    createBodyInScene(scene, scene_handles, state, child_body_index);
  }
}


void
removeBodyObjectFromScene(
  BodyIndex body_index,
  Scene &scene,
  SceneHandles &scene_handles
)
{
  scene.destroyTransformAndGeometry(scene_handles.body(body_index));
  scene_handles.bodies[body_index].reset();
}


void
createBodyBranchObjectsInScene(
  BodyIndex body_index,
  Scene &scene,
  SceneHandles &scene_handles,
  const SceneState &scene_state
)
{
  struct Visitor {
    Scene &scene;
    SceneHandles &scene_handles;
    const SceneState &scene_state;

    void visitBody(BodyIndex body_index) const
    {
      createBodyObjectInScene(
        body_index, scene, scene_handles, scene_state
      );
    }

    void visitMarker(MarkerIndex marker_index) const
    {
      createMarkerObjectInScene(
        marker_index, scene, scene_handles, scene_state
      );
    }
  } visitor = {scene, scene_handles, scene_state};

  forEachBranchIndexInPreOrder(body_index, scene_state, visitor);
}


void
removeBodyBranchObjectsFromScene(
  BodyIndex body_index,
  Scene &scene,
  SceneHandles &scene_handles,
  const SceneState &scene_state
)
{
  struct Visitor {
    Scene &scene;
    SceneHandles &scene_handles;

    void visitBody(BodyIndex body_index) const
    {
      removeBodyObjectFromScene(body_index, scene, scene_handles);
    }

    void visitMarker(MarkerIndex marker_index) const
    {
      removeMarkerObjectFromScene(marker_index, scene, scene_handles);
    }
  } visitor = {scene, scene_handles};

  forEachBranchIndexInPostOrder(body_index, scene_state, visitor);
}


void
removeBodyFromScene(
  Scene &scene,
  SceneHandles &scene_handles,
  const SceneState &state,
  BodyIndex body_index
)
{
  assert(!state.bodyHasChildren(body_index));
  removeBodyObjectFromScene(body_index, scene, scene_handles);
  removeIndexFrom(scene_handles.bodies, body_index);
}


SceneHandles createSceneObjects(const SceneState &state, Scene &scene)
{
  SceneHandles scene_handles;

  for (auto body_index : indicesOfChildBodies({}, state)) {
    createBodyInScene(scene, scene_handles, state, body_index);
  }

  for (auto marker_index : indicesOfMarkersOnBody({}, state)) {
    createMarkerInScene(scene, scene_handles, state, marker_index);
  }

  for (DistanceErrorIndex i : indicesOf(state.distance_errors)) {
    createDistanceErrorInScene(scene, scene_handles, state, i);
  }

  return scene_handles;
}


template <typename F>
static void
forEachChild(
  Optional<BodyIndex> maybe_parent_index,
  const SceneState &scene_state,
  const F &f
)
{
  for (BodyIndex body_index : indicesOf(scene_state.bodies())) {
    if (scene_state.body(body_index).maybe_parent_index == maybe_parent_index) {
      f(body_index);
    }
  }
}


static void
destroyBody(
  BodyIndex body_index,
  Scene &scene,
  const SceneState &scene_state,
  const SceneHandles &scene_handles
);


static void
destroyChildrenOf(
  Optional<BodyIndex> maybe_parent_index,
  Scene &scene,
  const SceneState &scene_state,
  const SceneHandles &scene_handles
)
{
  forEachChild(maybe_parent_index, scene_state, [&](BodyIndex child_index){
    destroyBody(child_index, scene, scene_state, scene_handles);
  });
}


static void
destroyBody(
  BodyIndex body_index,
  Scene &scene,
  const SceneState &scene_state,
  const SceneHandles &scene_handles
)
{
  destroyChildrenOf(body_index, scene, scene_state, scene_handles);
  scene.destroyTransformAndGeometry(scene_handles.body(body_index));
}


void
destroySceneObjects(
  Scene &scene,
  const SceneState &scene_state,
  const SceneHandles &scene_handles
)
{
  for (auto marker_index : indicesOf(scene_handles.markers)) {
    scene.destroyTransformAndGeometry(
      scene_handles.marker(marker_index).handle
    );
  }

  for (const auto &distance_error : scene_handles.distance_errors) {
    scene.destroyTransformAndGeometry(distance_error.line);
  }

  destroyChildrenOf({}, scene, scene_state, scene_handles);
}


static void
  updateDistanceErrorsInScene(
    Scene &scene,
    const SceneHandles &scene_handles,
    const SceneState &scene_state
  )
{
  for (auto i : indicesOf(scene_state.distance_errors)) {
    updateDistanceErrorInScene(scene, scene_state, scene_handles, i);
  }
}


static void
  updateMarkerInScene(
    Scene &scene,
    const SceneHandles::Marker &marker_handles,
    const SceneState::Marker &marker_state
  )
{
  scene.setTranslation(marker_handles.handle.transform_handle, makePoint(marker_state.position));
}


static void
updateMarkersInScene(
  Scene &scene,
  const SceneHandles &scene_handles,
  const SceneState::Markers &marker_states
)
{
  for (auto i : indicesOf(marker_states)) {
    updateMarkerInScene(scene, scene_handles.marker(i), marker_states[i]);
  }
}


static void
updateBodiesInScene(
  Scene &scene,
  const SceneState &state,
  const SceneHandles &scene_handles
)
{
  for (auto body_index : indicesOf(state.bodies())) {
    const BoxAndTransformHandle body_handle = scene_handles.body(body_index);
    updateBodyInScene(scene, body_handle, state.body(body_index));
  }
}


void
  updateSceneObjects(
    Scene &scene,
    const SceneHandles &scene_handles,
    const SceneState &state
  )
{
  updateBodiesInScene(scene, state, scene_handles);
  updateMarkersInScene(scene, scene_handles, state.markers());
  updateDistanceErrorsInScene(scene, scene_handles, state);
}
