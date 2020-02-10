#include "scenesolver.hpp"

#include <iostream>
#include "coordinateaxes.hpp"
#include "vec3.hpp"
#include "sceneerror.hpp"
#include "indicesof.hpp"
#include "randomengine.hpp"
#include "randomfloat.hpp"
#include "assertnearfloat.hpp"
#include "transform3.hpp"
#include "randomtransform3.hpp"
#include "randompoint3.hpp"
#include "transform3util.hpp"
#include "positionstatepoint3.hpp"
#include "randomvec3.hpp"

using std::cerr;


static SceneState::DistanceError& createDistanceError(SceneState &scene_state)
{
  DistanceErrorIndex index = scene_state.createDistanceError();
  return scene_state.distance_errors[index];
}


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


static MarkerIndex
  addMarkerTo(
    SceneState &result,
    const Point3 &local3,
    Optional<BodyIndex> maybe_body_index = {}
  )
{
  MarkerIndex marker_index = result.createUnnamedMarker();
  result.marker(marker_index).position = makePositionStateFromPoint3(local3);
  result.marker(marker_index).maybe_body_index = maybe_body_index;
  return marker_index;
}


namespace {
struct Example {
  SceneState scene_state;
  BodyIndex body_index;

  static BodyIndex createBody(SceneState &scene_state)
  {
    BodyIndex result = createGlobalBodyIn(scene_state);
    setAll(scene_state.body(result).solve_flags, true);
    return result;
  }

  Example()
  : scene_state(),
    body_index(createBody(scene_state))
  {
  }

  void addDistanceError(const Point3 &local3, const Point3 &global3)
  {
    MarkerIndex local_marker_index =
      addMarkerTo(scene_state, local3, body_index);

    MarkerIndex global_marker_index = addMarkerTo(scene_state, global3);

    SceneState::DistanceError &new_distance_error =
      createDistanceError(scene_state);

    new_distance_error.optional_start_marker_index = local_marker_index;
    new_distance_error.optional_end_marker_index = global_marker_index;
  }
};
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


static Example makeExample(RandomEngine &engine)
{
  Example result;
  SceneState &scene_state = result.scene_state;

  // Create a random transform and three random global points, then find the
  // equvalent local points and setup the scene from that.
  Transform3 true_box_global = randomTransform3(engine);
  Point3 global1 = randomPoint3(engine);
  Point3 global2 = randomPoint3(engine);
  Point3 global3 = randomPoint3(engine);
  Point3 local1 = localizePoint3(global1, true_box_global);
  Point3 local2 = localizePoint3(global2, true_box_global);
  Point3 local3 = localizePoint3(global3, true_box_global);
  result.addDistanceError(local1,global1);
  result.addDistanceError(local2,global2);
  result.addDistanceError(local3,global3);

  // Then we can set the box global transform to some other random
  // transform.

  SceneState::Body &body_state = scene_state.body(result.body_index);
  body_state.transform = randomUnscaledTransformState(engine);
  return result;
}


static void clearAll(SceneState::TransformSolveFlags &flags)
{
  setAll(flags, false);
}


static void testSolvingBoxTransform()
{
  RandomEngine engine(/*seed*/1);
  SceneState scene_state = makeExample(engine).scene_state;
  solveScene(scene_state);
  updateErrorsInState(scene_state);
  float error = sceneError(scene_state);
  assert(error < 0.003);
}


static void testSolvingBoxTransformWithoutXTranslation()
{
  RandomEngine engine(/*seed*/1);
  Example example = makeExample(engine);
  SceneState scene_state = example.scene_state;
  SceneState::Body &body_state = scene_state.body(example.body_index);
  float old_x_translation = body_state.transform.translation.x;
  body_state.solve_flags.translation.x = false;
  solveScene(scene_state);
  updateErrorsInState(scene_state);
  assert(sceneError(scene_state) >= 0.002);
  assert(body_state.transform.translation.x == old_x_translation);
}


static void testWithTwoBodies()
{
  SceneState scene_state;

  BodyIndex body1_index = createGlobalBodyIn(scene_state);

  BodyIndex body2_index =
    createBodyIn(scene_state, /*maybe_parent_index*/body1_index);

  MarkerIndex global_marker_index = scene_state.createMarker("global");
  MarkerIndex local_marker_index = scene_state.createMarker("global");

  SceneState::DistanceError &distance_error_state =
    createDistanceError(scene_state);

  distance_error_state.optional_start_marker_index = global_marker_index;
  distance_error_state.optional_end_marker_index = local_marker_index;

  scene_state.marker(local_marker_index).maybe_body_index = body2_index;

  for (auto body_index : indicesOf(scene_state.bodies())) {
    clearAll(scene_state.body(body_index).solve_flags);
  }

  scene_state.body(body1_index).solve_flags.translation.x = true;
  scene_state.body(body2_index).solve_flags.translation.y = true;
  float desired_x = 2;
  float desired_y = 3;
  scene_state.marker(global_marker_index).position.x = desired_x;
  scene_state.marker(global_marker_index).position.y = desired_y;
  solveScene(scene_state);
  float body1_x = scene_state.body(body1_index).transform.translation.x;
  float body2_y = scene_state.body(body2_index).transform.translation.y;

  float x_error = fabs(body1_x - desired_x);
  float y_error = fabs(body2_y - desired_y);

  float tolerance = 1e-4;

  if (x_error >= tolerance) {
    cerr << "x_error: " << x_error << "\n";
  }

  if (y_error >= tolerance) {
    cerr << "y_error: " << y_error << "\n";
  }

  assert(x_error <= tolerance);
  assert(y_error <= tolerance);
}


static void testSolvingScale()
{
  SceneState scene_state;

  // Create a body
  BodyIndex body_index = scene_state.createBody();

  // Create a local marker at (1,0,0)
  MarkerIndex local_marker_index = scene_state.createMarker(body_index);
  scene_state.marker(local_marker_index).position = {1,0,0};

  // Create a global marker at (2,0,0)
  MarkerIndex global_marker_index = scene_state.createMarker();
  scene_state.marker(global_marker_index).position = {2,0,0};

  // Create a distance error between the markers.
  DistanceErrorIndex distance_error_index = scene_state.createDistanceError();

  scene_state
    .distance_errors[distance_error_index]
    .optional_start_marker_index = local_marker_index;

  scene_state
    .distance_errors[distance_error_index]
    .optional_end_marker_index = global_marker_index;

  updateErrorsInState(scene_state);
  float old_scene_error = sceneError(scene_state);
  assert(old_scene_error != 0);

  // Set the scale solve flag on.
  scene_state.body(body_index).solve_flags.scale = true;

  // solve
  solveScene(scene_state);

  // Verify that the scale is 2
  SceneState::Float scale_value = scene_state.body(body_index).transform.scale;
  SceneState::Float expected_scale_value = 2;
  assertNear(scale_value, expected_scale_value, 1e-6);
}


int main()
{
  testSolvingBoxTransform();
  testSolvingBoxTransformWithoutXTranslation();
  testWithTwoBodies();
  testSolvingScale();
}
