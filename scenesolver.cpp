#include "scenesolver.hpp"

#include "sceneerror.hpp"
#include "vec3.hpp"
#include "maketransform.hpp"
#include "optimize.hpp"
#include "eigenconv.hpp"
#include "rotationvector.hpp"
#include "transformstate.hpp"


static void
  getValue(vector<float> &variables, float value, float scale, bool solve)
{
  if (solve) {
    variables.push_back(value*scale);
  }
}


static void
  updateValue(
    float &value,
    const vector<float> &variables,
    size_t &i,
    float inv_scale,
    bool solve
  )
{
  if (solve) {
    value = variables[i++]*inv_scale;
  }
}


static void
  getVariables(
    vector<float> &variables,
    const SceneState::XYZ &xyz,
    float scale,
    const SceneState::XYZSolveFlags &solve_flags
  )
{
  getValue(variables, xyz.x, scale, solve_flags.x);
  getValue(variables, xyz.y, scale, solve_flags.y);
  getValue(variables, xyz.z, scale, solve_flags.z);
}


static void
  updateValues(
    SceneState::XYZ &values,
    const vector<float> &variables,
    size_t &i,
    float inv_scale,
    const SceneState::XYZSolveFlags &solve_flags
  )
{
  updateValue(values.x, variables, i, inv_scale, solve_flags.x);
  updateValue(values.y, variables, i, inv_scale, solve_flags.y);
  updateValue(values.z, variables, i, inv_scale, solve_flags.z);
}


static vector<float>
  extractVariables(
    const TransformState &transform_state,
    const SceneState::TransformSolveFlags &solve_flags
  )
{
  vector<float> result;

  getVariables(
    result, transform_state.rotation, M_PI/180, solve_flags.rotation
  );

  getVariables(result, transform_state.translation, 1, solve_flags.translation);
  return result;
}


static void
  updateTransform(
    TransformState &transform_state,
    const vector<float> &variables,
    const SceneState::TransformSolveFlags &solve_flags
  )
{
  size_t i = 0;

  updateValues(
    transform_state.rotation, variables, i, 180/M_PI, solve_flags.rotation
  );

  updateValues(
    transform_state.translation, variables, i, 1, solve_flags.translation
  );

  assert(i == variables.size());
}


void solveBoxPosition(SceneState &scene_state)
{
  BodyIndex body_index = boxBodyIndex();
  SceneState::Body &body_state = scene_state.body(body_index);

  vector<float> variables =
    extractVariables(
      body_state.global,
      body_state.solve_flags
    );

  auto f = [&]{
    updateTransform(
      body_state.global,
      variables,
      body_state.solve_flags
    );

    updateErrorsInState(scene_state);
    return sceneError(scene_state);
  };

  minimize(f, variables);

  updateTransform(
    body_state.global,
    variables,
    body_state.solve_flags
  );
}
