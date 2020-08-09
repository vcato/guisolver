#include "globaltransform.hpp"

#include "randomengine.hpp"
#include "assertnearfloat.hpp"
#include "randomvec3.hpp"
#include "randomfloat.hpp"
#include "transformstate.hpp"
#include "vec3state.hpp"


static BodyIndex
  createBodyIn(SceneState &scene_state, Optional<BodyIndex> maybe_parent_index)
{
  BodyIndex body_index = scene_state.createBody(maybe_parent_index);
  scene_state.body(body_index).createBox();
  return body_index;
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


static void
assertNear(const Point &actual, const Point &expected, float tolerance)
{
  assertNear(actual.x(), expected.x(), tolerance);
  assertNear(actual.y(), expected.y(), tolerance);
  assertNear(actual.z(), expected.z(), tolerance);
}


static TransformState randomUnscaledTransformState(RandomEngine &engine)
{
  Vec3 t = randomVec3(engine);
  Vec3 r = randomVec3(engine);
  float scale = 1;

  return
    TransformState {
      {t.x, t.y, t.z},
      {r.x, r.y, r.z},
      scale
    };
}


static void testGlobalTransform()
{
  RandomEngine engine(/*seed*/1);
  // The marker predicted position on a body should be the same as the
  // local with the global transform of the body applied.

  SceneState scene_state;
  BodyIndex body1_index = scene_state.createBody();
  BodyIndex body2_index = scene_state.createBody(body1_index);

  scene_state.body(body1_index).transform =
    randomUnscaledTransformState(engine);

  scene_state.body(body2_index).transform =
    randomUnscaledTransformState(engine);

  MarkerIndex marker_index = scene_state.createMarker(body2_index);

  scene_state.marker(marker_index).position =
    makePositionStateFromVec3(randomVec3(engine));

  Point local = point(scene_state.marker(marker_index).position);

  assertNear(
    markerPredicted(scene_state, marker_index),
    Point(unscaledGlobalTransform(body2_index, scene_state)*local),
    0.001
  );
}


int main()
{
  testWithGlobalMarkerAtOrigin();
  testWithHierarchy();
  testGlobalTransform();
}
