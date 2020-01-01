#include "sceneobjects.hpp"

#include <iostream>
#include "settransform.hpp"
#include "maketransform.hpp"
#include "indicesof.hpp"
#include "removeindexfrom.hpp"
#include "transformstate.hpp"
#include "positionstate.hpp"

using std::cerr;
using TransformHandle = Scene::TransformHandle;
using LineHandle = Scene::LineHandle;

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
    makeMarkerPosition(localTranslation(handles_marker.handle, scene));
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
    const TransformHandle body_handle = scene_handles.body(i);
    SceneState::Body &body_state = state.body(i);
    body_state.transform = transformState(localTransform(scene, body_handle));
    body_state.geometry.scale = xyzState(scene.geometryScale(body_handle));
    body_state.geometry.center = xyzState(scene.geometryCenter(body_handle));
  }
}


static SceneHandles::Marker
  createSceneLocal(
    Scene &scene,
    TransformHandle &parent,
    Scene::Point position
  )
{
  auto point = scene.createSphere(parent);
  scene.setGeometryScale(point, {0.1, 0.1, 0.1});
  scene.setColor(point, 0, 0, 1);
  scene.setTranslation(point, position);
  SceneHandles::Marker marker_handles = SceneHandles::Marker{point};
  return marker_handles;
}


static SceneHandles::Marker
  createSceneGlobal(
    Scene &scene,
    Scene::Point position
  )
{
  auto point = scene.createSphere();
  scene.setGeometryScale(point, {0.1, 0.1, 0.1});
  scene.setColor(point, 0, 1, 0);
  scene.setTranslation(point, position);
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
    TransformHandle box_handle = scene_handles.body(parent_body_index);
    return createSceneLocal(scene, box_handle, position);
  }
  else {
    return createSceneGlobal(scene, position);
  }
}


static SceneHandles::DistanceError createDistanceError(Scene &scene)
{
  LineHandle line = scene.createLine(scene.top());
  scene.setColor(line,1,0,0);
  return {line};
}


static void
  updateDistanceErrorInScene(
    Scene &scene,
    const SceneHandles::DistanceError &distance_error_handles,
    const SceneState::DistanceError &distance_error_state,
    const SceneHandles &scene_handles
  )
{
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

    TransformHandle line_start =
      scene_handles.marker(start_marker_index).handle;

    TransformHandle line_end =
      scene_handles.marker(end_marker_index).handle;

    start = scene.worldPoint({0,0,0}, line_start);
    end = scene.worldPoint({0,0,0}, line_end);
  }

  scene.setStartPoint(distance_error_handles.line, start);
  scene.setEndPoint(distance_error_handles.line, end);
}


void
createDistanceErrorInScene(
  Scene &scene,
  SceneHandles &scene_handles,
  const SceneState &scene_state,
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

  updateDistanceErrorInScene(
    scene,
    scene_handles.distance_errors[index],
    scene_state.distance_errors[index],
    scene_handles
  );
}


void
  removeDistanceErrorFromScene(
    Scene &scene,
    SceneHandles::DistanceErrors &distance_errors,
    int index
  )
{
  scene.destroyLine(distance_errors[index].line);
  removeIndexFrom(distance_errors, index);
}


void
removeMarkerObjectFromScene(
  MarkerIndex index,
  Scene &scene,
  SceneHandles &scene_handles
)
{
  scene.destroyObject(scene_handles.marker(index).handle);
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


static TransformHandle
createBodyTransform(
  const SceneState::Body &body_state,
  Scene &scene,
  const SceneHandles &scene_handles
)
{
  if (body_state.maybe_parent_index) {
    const BodyIndex parent_body_index = *body_state.maybe_parent_index;

    TransformHandle parent_transform_handle =
      scene_handles.body(parent_body_index);

    return scene.createBox(parent_transform_handle);
  }
  else {
    return scene.createBox();
  }
}


static void
  updateBodyInScene(
    Scene &scene,
    const TransformHandle &body_transform_handle,
    const SceneState::Body &body_state
  )
{
  setTransform(
    body_transform_handle,
    makeTransformFromState(body_state.transform),
    scene
  );

  scene.setGeometryScale(
    body_transform_handle, vec3(body_state.geometry.scale)
  );

  scene.setGeometryCenter(
    body_transform_handle, point(body_state.geometry.center)
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

  TransformHandle transform_handle =
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
  scene.destroyObject(scene_handles.body(body_index));
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
  scene.destroyObject(scene_handles.body(body_index));
}


void
destroySceneObjects(
  Scene &scene,
  const SceneState &scene_state,
  const SceneHandles &scene_handles
)
{
  for (auto marker_index : indicesOf(scene_handles.markers)) {
    scene.destroyObject(scene_handles.marker(marker_index).handle);
  }

  for (const auto &distance_error : scene_handles.distance_errors) {
    scene.destroyLine(distance_error.line);
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
    updateDistanceErrorInScene(
      scene,
      scene_handles.distance_errors[i],
      scene_state.distance_errors[i],
      scene_handles
    );
  }
}


static void
  updateMarkerInScene(
    Scene &scene,
    const SceneHandles::Marker &marker_handles,
    const SceneState::Marker &marker_state
  )
{
  scene.setTranslation(marker_handles.handle, makePoint(marker_state.position));
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
    const TransformHandle body_handle = scene_handles.body(body_index);
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
