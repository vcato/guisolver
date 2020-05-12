#include "sceneobjects.hpp"

#include <iostream>
#include <float.h>
#include "settransform.hpp"
#include "indicesof.hpp"
#include "removeindexfrom.hpp"
#include "transformstate.hpp"
#include "positionstate.hpp"
#include "globaltransform.hpp"
#include "scenetransform.hpp"
#include "meshstate.hpp"
#include "vec3state.hpp"

using std::cerr;
using TransformHandle = Scene::TransformHandle;
using LineHandle = Scene::LineHandle;
using GeometryHandle = Scene::GeometryHandle;
using BodyState = SceneState::Body;


static Point
  localTranslation(Scene::TransformHandle transform_id,const Scene &scene)
{
  return makePointFromScenePoint(scene.translation(transform_id));
}


static SceneState::Float
bodyGlobalScale(BodyIndex body_index, const SceneState &scene_state)
{
  SceneState::Float scale = 1;

  for (;;) {
    const BodyState &body_state = scene_state.body(body_index);
    scale *= body_state.transform.scale;

    if (!body_state.maybe_parent_index) {
      break;
    }

    body_index = *body_state.maybe_parent_index;
  }

  return scale;
}


static void
updateMarkerPosition(
  SceneState::Marker &state_marker,
  const SceneHandles::Marker &marker_handles,
  const Scene &scene,
  const SceneState &scene_state,
  MarkerIndex marker_index
)
{
  Optional<BodyIndex> maybe_body_index =
    scene_state.marker(marker_index).maybe_body_index;

  float body_global_scale = 1;

  if (maybe_body_index) {
    body_global_scale = bodyGlobalScale(*maybe_body_index, scene_state);
  }

  state_marker.position =
    makePositionStateFromPoint(
      localTranslation(marker_handles.transformHandle(), scene)
      * (1 / body_global_scale)
    );
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
      scene_state.marker(i), scene_handles.marker(i), scene, scene_state, i
    );
  }
}


static Transform
localTransform(
  const Scene &scene,
  Scene::TransformHandle transform_handle,
  float parent_global_scale
)
{
  Point translation =
    localTranslation(transform_handle, scene) / parent_global_scale;

  CoordinateAxes coordinate_axes = scene.coordinateAxes(transform_handle);
  return makeTransform(coordinate_axes, translation);
}


namespace {
struct GeometryStateUpdater {
  SceneState::Body &body_state;
  const SceneHandles::Body &body_handles;
  const Scene &scene;
  const float body_global_scale;

  void visitBox(BoxIndex box_index)
  {
    SceneState::Box &box_state = body_state.boxes[box_index];
    Scene::GeometryHandle box_handle = body_handles.boxes[box_index].handle;

    Vec3 new_box_scale =
      scene.geometryScale(box_handle) *
      (1 / body_global_scale);

    box_state.scale = xyzStateFromVec3(new_box_scale);
    box_state.center = xyzStateFromVec3(scene.geometryCenter(box_handle));
  }

  void visitLine(LineIndex)
  {
  }

  void visitMesh(MeshIndex mesh_index)
  {
    SceneState::Mesh &mesh_state = body_state.meshes[mesh_index];
    Scene::GeometryHandle mesh_handle = body_handles.meshes[mesh_index].handle;

    Vec3 new_mesh_scale =
      scene.geometryScale(mesh_handle) *
      (1 / body_global_scale);

    mesh_state.scale = xyzStateFromVec3(new_mesh_scale);
    mesh_state.center = xyzStateFromVec3(scene.geometryCenter(mesh_handle));
  }
};
}


static void
updateBodyStateFromBodyObjects(
  SceneState::Body &body_state,
  const SceneHandles::Body &body_handles,
  const Scene &scene,
  BodyIndex body_index,
  const SceneState &scene_state
)
{
  Optional<BodyIndex> maybe_parent_body_index =
    scene_state.body(body_index).maybe_parent_index;

  float parent_global_scale = 1;

  if (maybe_parent_body_index) {
    parent_global_scale =
      bodyGlobalScale(*maybe_parent_body_index, scene_state);
  }

  // The translation needs to have the inverse scale applied.
  Transform local_transform =
    localTransform(scene, body_handles.transformHandle(), parent_global_scale);

  body_state.transform =
    transformState(local_transform, body_state.transform.scale);

  assert(body_state.boxes.size() == body_handles.boxes.size());
  float body_global_scale = bodyGlobalScale(body_index, scene_state);

  GeometryStateUpdater
    update_geometry_state({
      body_state,
      body_handles,
      scene,
      body_global_scale
    });

  forEachBodyGeometry(body_state, update_geometry_state);
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

    updateBodyStateFromBodyObjects(
      body_state, body_handles, scene, body_index, state
    );
  }
}


static Vec3
markerTranslation(
  MarkerIndex marker_index,
  const SceneState &scene_state
)
{
  float scale = 1;
  const SceneState::Markers &marker_states = scene_state.markers();
  const SceneState::Marker &marker_state = marker_states[marker_index];

  if (scene_state.marker(marker_index).maybe_body_index) {
    BodyIndex body_index = *scene_state.marker(marker_index).maybe_body_index;
    scale = bodyGlobalScale(body_index, scene_state);
  }

  Vec3 translation = 
    makeScenePointFromPoint(
      makePointFromPositionState(marker_state.position)*scale
    );

  return translation;
}


static SceneHandles::Marker
createMarker(
  TransformHandle parent,
  const Scene::Color &color,
  const SceneState &scene_state,
  Scene &scene,
  MarkerIndex marker_index
)
{
  TransformHandle transform_handle = scene.createTransform(parent);
  GeometryHandle sphere_handle = scene.createSphere(transform_handle);
  scene.setGeometryScale(sphere_handle, {0.1, 0.1, 0.1});
  scene.setGeometryColor(sphere_handle, color);
  Vec3 translation = markerTranslation(marker_index, scene_state);
  scene.setTranslation(transform_handle, translation);

  SceneHandles::Marker marker_handles =
    SceneHandles::Marker{transform_handle, sphere_handle};

  return marker_handles;
}


static SceneHandles::Marker
  createSceneMarker(
    Scene &scene,
    const SceneState::Marker &state_marker,
    const SceneState &scene_state,
    const SceneHandles &scene_handles,
    MarkerIndex marker_index
  )
{
  Optional<BodyIndex> maybe_body_index = state_marker.maybe_body_index;

  if (maybe_body_index) {
    BodyIndex parent_body_index = *maybe_body_index;

    const SceneHandles::Body &body_handles =
      scene_handles.body(parent_body_index);

    TransformHandle parent_handle = body_handles.transformHandle();
    Scene::Color color = {0, 0, 1};
    return createMarker(parent_handle, color, scene_state, scene, marker_index);
  }
  else {
    Scene::Color color = {0, 1, 0};
    return createMarker(scene.top(), color, scene_state, scene, marker_index);
  }
}


static SceneHandles::DistanceError createDistanceError(Scene &scene)
{
  TransformHandle transform_handle = scene.createTransform(scene.top());
  LineHandle line_handle = scene.createLine(transform_handle);
  scene.setGeometryColor(line_handle, {1,0,0});
  return {transform_handle, line_handle};
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
    distance_error_state.optional_start_marker &&
    distance_error_state.optional_end_marker;

  Point start = {0,0,0};
  Point end = {0,0,0};

  if (have_both_markers) {
    MarkerIndex start_marker_index =
      distance_error_state.optional_start_marker->index;

    MarkerIndex end_marker_index =
      distance_error_state.optional_end_marker->index;

    start = markerPredicted(scene_state, start_marker_index);
    end = markerPredicted(scene_state, end_marker_index);
  }

  scene.setLineStartPoint(
    distance_error_handles.line_handle, makeScenePointFromPoint(start)
  );

  scene.setLineEndPoint(
    distance_error_handles.line_handle, makeScenePointFromPoint(end)
  );
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
destroyLineObjects(const SceneHandles::Line &line_handles, Scene &scene)
{
  scene.destroyGeometry(line_handles.handle);
}


static void
destroyMeshObjects(const SceneHandles::Mesh &mesh_handles, Scene &scene)
{
  scene.destroyGeometry(mesh_handles.handle);
}


namespace {
struct GeometryDestroyer {
  const SceneHandles::Body &body_handles;
  Scene &scene;

  void visitBox(BoxIndex box_index)
  {
    destroyBoxObjects(body_handles.boxes[box_index], scene);
  }

  void visitMesh(MeshIndex mesh_index)
  {
    destroyMeshObjects(body_handles.meshes[mesh_index], scene);
  }

  void visitLine(LineIndex line_index)
  {
    destroyLineObjects(body_handles.lines[line_index], scene);
  }
};
}


static void
destroyBodyObjects(
  BodyIndex body_index,
  Scene &scene,
  const SceneHandles &scene_handles,
  const SceneState &scene_state
)
{
  const SceneHandles::Body &body_handles = scene_handles.body(body_index);
  GeometryDestroyer geometry_destroyer({body_handles, scene});
  forEachBodyGeometry(scene_state.body(body_index), geometry_destroyer);
  scene.destroyTransform(body_handles.transformHandle());
}


static void
destroyMarkerObjects(const SceneHandles::Marker &marker_handles, Scene &scene)
{
  scene.destroyGeometry(marker_handles.sphereHandle());
  scene.destroyTransform(marker_handles.transformHandle());
}


static void
destroyDistanceErrorObjects(
  const SceneHandles::DistanceError &distance_error_handles,
  Scene &scene
)
{
  scene.destroyGeometry(distance_error_handles.line_handle);
  scene.destroyTransform(distance_error_handles.transform_handle);
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


extern void
  removeVariableFromScene(
    Scene &,
    SceneHandles &,
    VariableIndex
  )
{
  // Nothing to do here since variables have no scene representation
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
    createSceneMarker(scene, state_marker, state, scene_handles, marker_index);
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
updateBoxInScene(
  Scene &scene,
  const SceneState::Box &box_state,
  const SceneHandles::Box &box_handles,
  SceneState::Float body_global_scale
)
{
  scene.setGeometryScale(
    box_handles.handle, vec3FromXYZState(box_state.scale) * body_global_scale
  );

  scene.setGeometryCenter(
    box_handles.handle,
    vec3FromXYZState(box_state.center)*body_global_scale
  );
}


static void
updateLineInScene(
  Scene &scene,
  const SceneState::Line &line_state,
  const SceneHandles::Line &line_handles,
  SceneState::Float body_global_scale
)
{
  scene.setLineStartPoint(
    line_handles.handle, vec3FromXYZState(line_state.start)*body_global_scale
  );

  scene.setLineEndPoint(
    line_handles.handle, vec3FromXYZState(line_state.end)*body_global_scale
  );
}


static void
updateMeshInScene(
  Scene &scene,
  const SceneState::Mesh &mesh_state,
  const SceneHandles::Mesh &mesh_handles,
  SceneState::Float body_global_scale
)
{
  scene.setGeometryScale(
    mesh_handles.handle, vec3FromXYZState(mesh_state.scale) * body_global_scale
  );

  scene.setGeometryCenter(
    mesh_handles.handle, vec3FromXYZState(mesh_state.center) * body_global_scale
  );
}


namespace {
struct GeometrySceneUpdater {
  const SceneHandles::Body &body_handles;
  const SceneState::Body &body_state;
  Scene &scene;
  const float body_global_scale;

  void visitBox(BoxIndex box_index)
  {
    const SceneHandles::Box &box_handles = body_handles.boxes[box_index];
    const SceneState::Box &box_state = body_state.boxes[box_index];
    updateBoxInScene(scene, box_state, box_handles, body_global_scale);
  }

  void visitLine(LineIndex line_index)
  {
    const SceneHandles::Line &line_handles = body_handles.lines[line_index];
    const SceneState::Line &line_state = body_state.lines[line_index];
    updateLineInScene(scene, line_state, line_handles, body_global_scale);
  }

  void visitMesh(MeshIndex mesh_index)
  {
    const SceneHandles::Mesh &mesh_handles = body_handles.meshes[mesh_index];
    const SceneState::Mesh &mesh_state = body_state.meshes[mesh_index];
    updateMeshInScene(scene, mesh_state, mesh_handles, body_global_scale);
  }
};
}


static void
updateBodyInScene(
  Scene &scene,
  BodyIndex body_index,
  const SceneState &scene_state,
  const SceneHandles &scene_handles
)
{
  const SceneState::Body &body_state = scene_state.body(body_index);
  const SceneHandles::Body &body_handles = *scene_handles.bodies[body_index];
  float parent_global_scale = 1;

  if (body_state.maybe_parent_index) {
    parent_global_scale =
      bodyGlobalScale(*body_state.maybe_parent_index, scene_state);
  }

  float body_global_scale = parent_global_scale * body_state.transform.scale;

  setTransform(
    body_handles.transformHandle(),
    makeUnscaledTransformFromState(
      body_state.transform, parent_global_scale
    ),
    scene
  );

  assert(body_handles.boxes.size() == body_state.boxes.size());

  GeometrySceneUpdater geometry_scene_updater({
    body_handles,
    body_state,
    scene,
    body_global_scale
  });

  forEachBodyGeometry(body_state, geometry_scene_updater);
}


void
createBoxInScene(
  Scene &scene,
  SceneHandles &scene_handles,
  BodyIndex body_index,
  BoxIndex
#ifndef NDEBUG
    box_index
#endif
)
{
  SceneHandles::Body &body_handles = *scene_handles.bodies[body_index];
  TransformHandle transform_handle = body_handles.transform_handle;
  GeometryHandle geometry_handle = scene.createBox(transform_handle);
  assert(BoxIndex(body_handles.boxes.size()) == box_index);
  body_handles.addBox(geometry_handle);
}


void
createLineInScene(
  Scene &scene,
  SceneHandles &scene_handles,
  BodyIndex body_index,
  LineIndex line_index,
  const SceneState &scene_state
)
{
  SceneHandles::Body &body_handles = *scene_handles.bodies[body_index];
  TransformHandle transform_handle = body_handles.transform_handle;
  LineHandle line_handle = scene.createLine(transform_handle);
  assert(LineIndex(body_handles.lines.size()) == line_index);
  body_handles.addLine(line_handle);

  updateLineInScene(
    scene,
    scene_state.body(body_index).lines[line_index],
    scene_handles.body(body_index).lines[line_index],
    bodyGlobalScale(body_index, scene_state)
  );
}


void
createMeshInScene(
  Scene &scene,
  SceneHandles &scene_handles,
  BodyIndex parent_body_index,
  MeshIndex mesh_index,
  const SceneState &scene_state
)
{
  SceneHandles::Body &body_handles = *scene_handles.bodies[parent_body_index];
  TransformHandle transform_handle = body_handles.transform_handle;

  const SceneState::Mesh &mesh =
    scene_state.body(parent_body_index).meshes[mesh_index];

  Scene::MeshHandle mesh_handle =
    scene.createMesh(transform_handle, meshFromMeshShapeState(mesh.shape));

  assert(BodyIndex(body_handles.meshes.size()) == mesh_index);

  body_handles.addMesh(mesh_handle);
}



static void
createBodyObjectInScene(
  BodyIndex body_index,
  Scene &scene,
  SceneHandles &scene_handles,
  const SceneState &scene_state
)
{
  const SceneState::Body &body_state = scene_state.body(body_index);
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
    createBoxInScene(scene, scene_handles, body_index, i);
  }

  size_t n_lines = body_state.lines.size();

  for (size_t i=0; i!=n_lines; ++i) {
    createLineInScene(scene, scene_handles, body_index, i, scene_state);
  }

  size_t n_meshes = body_state.meshes.size();

  for (size_t i=0; i!=n_meshes; ++i) {
    createMeshInScene(scene, scene_handles, body_index, i, scene_state);
  }

  updateBodyInScene(scene, body_index, scene_state, scene_handles);
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

  for (auto i : indicesOfMarkersOnBody(body_index, state)) {
    createMarkerInScene(scene, scene_handles, state, i);
  }

  for (auto i : indicesOfChildBodies(body_index, state)) {
    createBodyInScene(scene, scene_handles, state, i);
  }

  for (auto i : indicesOfDistanceErrorsOnBody(body_index, state)) {
    createDistanceErrorInScene(scene, scene_handles, state, i);
  }
}


static void
removeBodyObjectFromScene(
  BodyIndex body_index,
  Scene &scene,
  SceneHandles &scene_handles,
  const SceneState &scene_state
)
{
  destroyBodyObjects(body_index, scene, scene_handles, scene_state);
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
    const SceneState &scene_state;

    void visitBody(BodyIndex body_index) const
    {
      removeBodyObjectFromScene(body_index, scene, scene_handles, scene_state);
    }

    void visitMarker(MarkerIndex marker_index) const
    {
      removeMarkerObjectFromScene(marker_index, scene, scene_handles);
    }
  } visitor = {scene, scene_handles, scene_state};

  forEachBranchIndexInPostOrder(body_index, scene_state, visitor);
}


void
removeBodyFromScene(
  Scene &scene,
  SceneHandles &scene_handles,
  const SceneState &scene_state,
  BodyIndex body_index
)
{
  assert(!scene_state.bodyHasChildren(body_index));
  removeBodyObjectFromScene(body_index, scene, scene_handles, scene_state);
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


void
removeLineFromScene(
  Scene &scene,
  SceneHandles &scene_handles,
  const SceneState &,
  BodyIndex body_index,
  LineIndex line_index
)
{
  SceneHandles::Body &body_handles = *scene_handles.bodies[body_index];
  destroyLineObjects(body_handles.lines[line_index], scene);
  removeIndexFrom(body_handles.lines, line_index);
}


void
removeMeshFromScene(
  Scene &scene,
  SceneHandles &scene_handles,
  const SceneState &,
  BodyIndex body_index,
  MeshIndex mesh_index
)
{
  SceneHandles::Body &body_handles = *scene_handles.bodies[body_index];
  destroyMeshObjects(body_handles.meshes[mesh_index], scene);
  removeIndexFrom(body_handles.meshes, mesh_index);
}


SceneHandles createSceneObjects(const SceneState &state, Scene &scene)
{
  SceneHandles scene_handles;

  for (auto i : indicesOfChildBodies({}, state)) {
    createBodyInScene(scene, scene_handles, state, i);
  }

  for (auto i : indicesOfMarkersOnBody({}, state)) {
    createMarkerInScene(scene, scene_handles, state, i);
  }

  for (auto i : indicesOfDistanceErrorsOnBody({}, state)) {
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
  destroyBodyObjects(body_index, scene, scene_handles, scene_state);
}


void removeExistingManipulator(SceneHandles &scene_handles, Scene &scene)
{
  if (scene_handles.maybe_translate_manipulator) {
    TransformHandle manipulator_transform =
      *scene_handles.maybe_translate_manipulator;

    scene_handles.maybe_translate_manipulator.reset();

    if (scene_handles.maybe_manipulated_element.maybe_body_index) {
      scene_handles.maybe_manipulated_element.maybe_body_index.reset();
    }
    else if (scene_handles.maybe_manipulated_element.maybe_marker_index) {
      scene_handles.maybe_manipulated_element.maybe_marker_index.reset();
    }
    else if (scene_handles.maybe_manipulated_element.maybe_body_mesh_position) {
      scene_handles.maybe_manipulated_element.maybe_body_mesh_position.reset();
    }
    else {
      assert(false); // not implemented
    }

    scene.destroyTransform(manipulator_transform);
  }
  else if (scene_handles.maybe_rotate_manipulator) {
    TransformHandle manipulator_transform =
      *scene_handles.maybe_rotate_manipulator;

    scene_handles.maybe_rotate_manipulator.reset();

    if (scene_handles.maybe_manipulated_element.maybe_body_index) {
      scene_handles.maybe_manipulated_element.maybe_body_index.reset();
    }
    else {
      assert(false); // not implemented
    }

    scene.destroyTransform(manipulator_transform);
  }
  else if (scene_handles.maybe_scale_manipulator) {
    GeometryHandle manipulator = *scene_handles.maybe_scale_manipulator;

    if (scene_handles.maybe_manipulated_element.maybe_body_box) {
      scene_handles.maybe_manipulated_element.maybe_body_box.reset();
    }
    else if (scene_handles.maybe_manipulated_element.maybe_body_mesh) {
      scene_handles.maybe_manipulated_element.maybe_body_mesh.reset();
    }
    else {
      assert(false); // not implemented
    }

    scene_handles.maybe_scale_manipulator.reset();
    scene.destroyGeometry(manipulator);
  }

  if (scene_handles.maybe_points) {
    const vector<Scene::GeometryHandle> &handles =
      *scene_handles.maybe_points;

    for (auto handle : handles) {
      scene.destroyGeometry(handle);
    }

    scene_handles.maybe_points.reset();
  }
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


void
updateBodyTranslateManipulator(
  Scene &scene,
  TransformHandle body_transform_handle,
  TransformHandle manipulator_handle
)
{
  auto body_position = scene.translation(body_transform_handle);
  scene.setTranslation(manipulator_handle, body_position);
}


void
updateBodyRotateManipulator(
  Scene &scene,
  Scene::TransformHandle body_transform_handle,
  Scene::TransformHandle manipulator_handle
)
{
  auto body_position = scene.translation(body_transform_handle);
  auto body_orientation = scene.coordinateAxes(body_transform_handle);
  scene.setTranslation(manipulator_handle, body_position);
  scene.setCoordinateAxes(manipulator_handle, body_orientation);
}


void
updateMarkerTranslateManipulator(
  Scene &scene,
  TransformHandle marker_transform_handle,
  TransformHandle manipulator_handle
)
{
  auto marker_position = scene.translation(marker_transform_handle);
  scene.setTranslation(manipulator_handle, marker_position);
}


void
updateBodyBoxScaleManipulator(
  Scene &scene,
  Scene::GeometryHandle box_geometry_handle,
  Scene::GeometryHandle manipulator
)
{
  auto center = scene.geometryCenter(box_geometry_handle);
  scene.setGeometryCenter(manipulator, center);
  auto size = scene.geometryScale(box_geometry_handle);
  size.x += 0.1;
  size.y += 0.1;
  size.z += 0.1;
  scene.setGeometryScale(manipulator, size);
}


void
updateBodyBoxFromScaleManipulator(
  GeometryHandle box_handle,
  GeometryHandle manipulator,
  Scene &scene
)
{
  Scene::Point center = scene.geometryCenter(manipulator);
  Vec3 scale = scene.geometryScale(manipulator);
  scale.x -= 0.1;
  scale.y -= 0.1;
  scale.z -= 0.1;

  scene.setGeometryCenter(box_handle, center);
  scene.setGeometryScale(box_handle, scale);
}


namespace {
struct Range {
  float min = FLT_MAX, max = -FLT_MAX;
};
}


static void expand(Range &range, float value)
{
  if (value < range.min) {
    range.min = value;
  }

  if (value > range.max) {
    range.max = value;
  }
}


static Vec3 meshSize(const ::Mesh &mesh)
{
  Range x_range;
  Range y_range;
  Range z_range;

  for (auto &position : mesh.positions) {
    expand(x_range, position.x);
    expand(y_range, position.y);
    expand(z_range, position.z);
  }

  float x = std::max(-x_range.min, x_range.max);
  float y = std::max(-y_range.min, y_range.max);
  float z = std::max(-z_range.min, z_range.max);

  return {x,y,z};
}


void
updateBodyMeshScaleManipulator(
  Scene &scene,
  Scene::MeshHandle mesh_handle,
  Scene::GeometryHandle manipulator
)
{
  auto center = scene.geometryCenter(mesh_handle);
  scene.setGeometryCenter(manipulator, center);
  auto size = scene.geometryScale(mesh_handle);
  const Mesh &mesh = scene.mesh(mesh_handle);
  Vec3 mesh_size = meshSize(mesh);
  size.x *= mesh_size.x*2;
  size.y *= mesh_size.y*2;
  size.z *= mesh_size.z*2;
  scene.setGeometryScale(manipulator, size);
}


void
updateBodyMeshFromScaleManipulator(
  Scene::MeshHandle mesh_handle,
  GeometryHandle manipulator,
  Scene &scene
)
{
  Scene::Point center = scene.geometryCenter(manipulator);
  Vec3 scale = scene.geometryScale(manipulator);
  const Mesh &mesh = scene.mesh(mesh_handle);
  Vec3 mesh_size = meshSize(mesh);
  scale.x /= mesh_size.x*2;
  scale.y /= mesh_size.y*2;
  scale.z /= mesh_size.z*2;
  scene.setGeometryCenter(mesh_handle, center);
  scene.setGeometryScale(mesh_handle, scale);
}


static void
updateMarkerInScene(
  Scene &scene,
  const SceneHandles &scene_handles,
  const SceneState &scene_state,
  MarkerIndex marker_index
)
{
  Vec3 translation = markerTranslation(marker_index, scene_state);

  const SceneHandles::Marker &marker_handles =
    scene_handles.marker(marker_index);

  scene.setTranslation(marker_handles.transformHandle(), translation);
}


static void
updateMarkersInScene(
  Scene &scene,
  const SceneHandles &scene_handles,
  const SceneState &scene_state
)
{
  for (auto i : indicesOf(scene_state.markers())) {
    updateMarkerInScene(scene, scene_handles, scene_state, i);
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
    updateBodyInScene(scene, body_index, state, scene_handles);
  }
}


static void
updateManipulatorsInScene(
  Scene &scene,
  const SceneHandles &scene_handles
)
{
  if (scene_handles.maybe_manipulated_element.maybe_body_box) {
    if (scene_handles.maybe_scale_manipulator) {
      BodyBox body_box = *scene_handles.maybe_manipulated_element.maybe_body_box;
      GeometryHandle manipulator = *scene_handles.maybe_scale_manipulator;
      BodyIndex body_index = body_box.body.index;
      BoxIndex box_index = body_box.index;

      GeometryHandle box_geometry_handle =
        scene_handles.body(body_index).boxes[box_index].handle;

      updateBodyBoxScaleManipulator(
        scene,
        box_geometry_handle,
        manipulator
      );
    }
  }
  else if (scene_handles.maybe_manipulated_element.maybe_body_mesh) {
    BodyMesh body_mesh = *scene_handles.maybe_manipulated_element.maybe_body_mesh;
    BodyIndex body_index = body_mesh.body.index;
    MeshIndex mesh_index = body_mesh.index;
    GeometryHandle manipulator = *scene_handles.maybe_scale_manipulator;

    Scene::MeshHandle mesh_handle =
      scene_handles.body(body_index).meshes[mesh_index].handle;

    updateBodyMeshScaleManipulator(scene, mesh_handle, manipulator);
  }
  else if (scene_handles.maybe_manipulated_element.maybe_marker_index) {
    MarkerIndex marker_index = *scene_handles.maybe_manipulated_element.maybe_marker_index;

    if (scene_handles.maybe_translate_manipulator) {
      TransformHandle manipulator = *scene_handles.maybe_translate_manipulator;

      const SceneHandles::Marker &marker_handles =
        scene_handles.marker(marker_index);

      TransformHandle marker_transform_handle =
        marker_handles.transformHandle();

      updateMarkerTranslateManipulator(
        scene,
        marker_transform_handle,
        manipulator
      );
    }
  }
  else if (scene_handles.maybe_manipulated_element.maybe_body_index) {
    BodyIndex body_index = *scene_handles.maybe_manipulated_element.maybe_body_index;

    if (scene_handles.maybe_translate_manipulator) {
      TransformHandle manipulator = *scene_handles.maybe_translate_manipulator;

      updateBodyTranslateManipulator(
        scene,
        scene_handles.body(body_index).transform_handle,
        manipulator
      );
    }
    else if (scene_handles.maybe_rotate_manipulator) {
      TransformHandle manipulator = *scene_handles.maybe_rotate_manipulator;

      updateBodyRotateManipulator(
        scene,
        scene_handles.body(body_index).transform_handle,
        manipulator
      );
    }
    else {
      cerr << "updateManipulatorsInScene: unknown body manipulator\n";
    }
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
  updateMarkersInScene(scene, scene_handles, state);
  updateDistanceErrorsInScene(scene, scene_handles, state);
  updateManipulatorsInScene(scene, scene_handles);
}


void
updateBodyMeshPositionInScene(
  BodyIndex body_index,
  MeshIndex mesh_index,
  MeshPositionIndex /*mesh_position_index*/,
  Scene &scene,
  const SceneHandles &scene_handles,
  const SceneState &scene_state
)
{
  Scene::MeshHandle mesh_handle =
    scene_handles.bodies[body_index]->meshes[mesh_index].handle;

  const SceneState::Mesh &mesh_state =
    scene_state.body(body_index).meshes[mesh_index];

  scene.setMesh(mesh_handle, meshFromMeshShapeState(mesh_state.shape));
}


Vec3
bodyMeshPositionRelativeToBody(
  BodyMeshPosition body_mesh_position,
  const SceneState &scene_state
)
{
  BodyIndex body_index = body_mesh_position.array.body_mesh.body.index;
  MeshIndex mesh_index = body_mesh_position.array.body_mesh.index;
  MeshPositionIndex position_index = body_mesh_position.index;

  const SceneState::Mesh &mesh_state =
    scene_state.body(body_index).meshes[mesh_index];

  Vec3 scale = vec3FromXYZState(mesh_state.scale);
  Vec3 center = vec3FromXYZState(mesh_state.center);

  Vec3 position =
    vec3FromXYZState(mesh_state.shape.positions[position_index]);

  Vec3 scaled_position =
    Vec3(
      center.x + position.x * scale.x,
      center.y + position.y * scale.y,
      center.z + position.z * scale.z
    );

  return scaled_position;
}


void
updateBodyMeshPositionManipulator(
  TransformHandle manipulator,
  BodyMeshPosition body_mesh_position,
  const SceneState &scene_state,
  Scene &scene
)
{
  Vec3 scaled_position =
    bodyMeshPositionRelativeToBody(body_mesh_position, scene_state);

  scene.setTranslation(manipulator, scaled_position);
}


void
updateBodyMeshPositionFromManipulator(
  TransformHandle manipulator,
  BodyMeshPosition body_mesh_position,
  SceneState &scene_state,
  Scene &scene,
  const SceneHandles &scene_handles
)
{
  BodyIndex body_index = body_mesh_position.array.body_mesh.body.index;
  MeshIndex mesh_index = body_mesh_position.array.body_mesh.index;
  MeshPositionIndex position_index = body_mesh_position.index;

  Vec3 scaled_position = scene.translation(manipulator);

  SceneState::Mesh &mesh_state =
    scene_state.body(body_index).meshes[mesh_index];

  Vec3 scale = vec3FromXYZState(mesh_state.scale);
  Vec3 center = vec3FromXYZState(mesh_state.center);

  Vec3 position = {
    (scaled_position.x - center.x)/scale.x,
    (scaled_position.y - center.y)/scale.y,
    (scaled_position.z - center.z)/scale.z
  };

  mesh_state.shape.positions[position_index] = xyzStateFromVec3(position);

  updateBodyMeshPositionInScene(
    body_index,
    mesh_index,
    position_index,
    scene,
    scene_handles,
    scene_state
  );
}
