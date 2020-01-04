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
using GeometryHandle = Scene::GeometryHandle;
using BoxAndTransformHandle = Scene::BoxAndTransformHandle;
using GeometryAndTransformHandle = Scene::GeometryAndTransformHandle;
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


static void
updateBodyStateFromBodyObjects(
  SceneState::Body &body_state,
  const SceneHandles::Body &body_handles,
  const Scene &scene
)
{
  body_state.transform =
    transformState(localTransform(scene, body_handles.transformHandle()));

  assert(body_state.boxes.size() == body_handles.boxes.size());
  size_t n_boxes = body_state.boxes.size();

  for (size_t box_index=0; box_index!=n_boxes; ++box_index) {
    SceneState::Box &box_state = body_state.boxes[box_index];
    Scene::GeometryHandle box_handle = body_handles.boxes[box_index].handle;
    box_state.scale = xyzState(scene.geometryScale(box_handle));
    box_state.center = xyzState(scene.geometryCenter(box_handle));
  }
}


void
updateSceneStateFromSceneObjects(
  SceneState &state,
  const Scene &scene,
  const SceneHandles &scene_handles
)
{
  updateStateMarkerPositions(state, scene_handles, scene);

  for (auto body_index : indicesOf(state.bodies())) {
    const SceneHandles::Body &body_handles = scene_handles.body(body_index);
    SceneState::Body &body_state = state.body(body_index);
    updateBodyStateFromBodyObjects(body_state, body_handles, scene);
  }
}


static SceneHandles::Marker
  createSceneLocal(
    Scene &scene,
    TransformHandle const parent,
    Scene::Point position
  )
{
  auto point = scene.createSphereAndTransform(parent);
  scene.setGeometryScale(point.geometry_handle, {0.1, 0.1, 0.1});
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
  scene.setGeometryScale(point.geometry_handle, {0.1, 0.1, 0.1});
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

    const SceneHandles::Body &body_handles =
      scene_handles.body(parent_body_index);

    return createSceneLocal(scene, body_handles.transformHandle(), position);
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


static void
destroyBoxObjects(const SceneHandles::Box &box_handles, Scene &scene)
{
  scene.destroyGeometry(box_handles.handle);
}


static void
destroyBodyObjects(
  BodyIndex body_index, Scene &scene, const SceneHandles &scene_handles
)
{
  const SceneHandles::Body &body_handles = scene_handles.body(body_index);

  {
    size_t n_boxes = body_handles.boxes.size();

    for (size_t box_index = 0; box_index != n_boxes; ++box_index) {
      destroyBoxObjects(body_handles.boxes[box_index], scene);
    }
  }

  scene.destroyTransform(body_handles.transformHandle());
}


static void
destroyMarkerObjects(const SceneHandles::Marker &marker_handles, Scene &scene)
{
  scene.destroyGeometry(marker_handles.handle.geometry_handle);
  scene.destroyTransform(marker_handles.handle.transform_handle);
}


static void
destroyDistanceErrorObjects(
  const SceneHandles::DistanceError &distance_error_handles,
  Scene &scene
)
{
  scene.destroyGeometry(distance_error_handles.line.geometry_handle);
  scene.destroyTransform(distance_error_handles.line.transform_handle);
}


void
  removeDistanceErrorFromScene(
    Scene &scene,
    SceneHandles::DistanceErrors &distance_errors,
    int index
  )
{
  destroyDistanceErrorObjects(distance_errors[index], scene);
  removeIndexFrom(distance_errors, index);
}


void
removeMarkerObjectFromScene(
  MarkerIndex index,
  Scene &scene,
  SceneHandles &scene_handles
)
{
  destroyMarkerObjects(scene_handles.marker(index), scene);
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


static void
  updateBodyInScene(
    Scene &scene,
    const SceneState::Body &body_state,
    const SceneHandles::Body &body_handles
  )
{
  setTransform(
    body_handles.transformHandle(),
    makeTransformFromState(body_state.transform),
    scene
  );

  assert(body_handles.boxes.size() == body_state.boxes.size());
  size_t n_boxes = body_state.boxes.size();

  for (size_t box_index = 0; box_index != n_boxes; ++box_index) {
    const SceneHandles::Box &box_handles = body_handles.boxes[box_index];
    const SceneState::Box &box_state = body_state.boxes[box_index];
    scene.setGeometryScale(box_handles.handle, vec3(box_state.scale));
    scene.setGeometryCenter(box_handles.handle, point(box_state.center));
  }
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
  assert(!scene_handles.bodies[body_index].hasValue());
  Optional<TransformHandle> maybe_parent_transform;

  if (body_state.maybe_parent_index) {
    const BodyIndex parent_body_index = *body_state.maybe_parent_index;

    const SceneHandles::Body &parent_body_handles =
      scene_handles.body(parent_body_index);

    maybe_parent_transform = parent_body_handles.transformHandle();
  }
  else {
    maybe_parent_transform = scene.top();
  }

  const TransformHandle &parent_transform = *maybe_parent_transform;

  TransformHandle transform_handle = scene.createTransform(parent_transform);
  scene_handles.bodies[body_index] = SceneHandles::Body{transform_handle};
  size_t n_boxes = body_state.boxes.size();

  for (size_t i=0; i!=n_boxes; ++i) {
    GeometryHandle geometry_handle = scene.createBox(transform_handle);
    scene_handles.bodies[body_index]->addBox(geometry_handle);
  }

  updateBodyInScene(scene, body_state, *scene_handles.bodies[body_index]);
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
  destroyBodyObjects(body_index, scene, scene_handles);
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


void
removeBoxFromScene(
  Scene &scene,
  SceneHandles &scene_handles,
  const SceneState &,
  BodyIndex body_index,
  BoxIndex box_index
)
{
  SceneHandles::Body &body_handles = *scene_handles.bodies[body_index];
  destroyBoxObjects(body_handles.boxes[box_index], scene);
  removeIndexFrom(body_handles.boxes, box_index);
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
destroyBodyAndChildren(
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
    destroyBodyAndChildren(child_index, scene, scene_state, scene_handles);
  });
}


static void
destroyBodyAndChildren(
  BodyIndex body_index,
  Scene &scene,
  const SceneState &scene_state,
  const SceneHandles &scene_handles
)
{
  destroyChildrenOf(body_index, scene, scene_state, scene_handles);
  destroyBodyObjects(body_index, scene, scene_handles);
}


void
destroySceneObjects(
  Scene &scene,
  const SceneState &scene_state,
  const SceneHandles &scene_handles
)
{
  for (const auto marker_index : indicesOf(scene_handles.markers)) {
    destroyMarkerObjects(scene_handles.marker(marker_index), scene);
  }

  for (const auto &distance_error : scene_handles.distance_errors) {
    destroyDistanceErrorObjects(distance_error, scene);
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
    const SceneHandles::Body &body_handles =
      scene_handles.body(body_index);

    updateBodyInScene(scene, state.body(body_index), body_handles);
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
