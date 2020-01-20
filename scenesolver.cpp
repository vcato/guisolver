#include "scenesolver.hpp"

#include "sceneerror.hpp"
#include "vec3.hpp"
#include "maketransform.hpp"
#include "optimize.hpp"
#include "eigenconv.hpp"
#include "rotationvector.hpp"
#include "transformstate.hpp"
#include "indicesof.hpp"


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


template <typename XYZ, typename F>
static void
  forEachXYZValue(
    XYZ &xyz,
    const SceneState::XYZSolveFlags &solve_flags,
    const F &f
  )
{
  f(xyz.x, solve_flags.x);
  f(xyz.y, solve_flags.y);
  f(xyz.z, solve_flags.z);
}


template <typename TransformState, typename F>
static void
  forEachTransformValue(
    TransformState &transform_state,
    const SceneState::TransformSolveFlags &solve_flags,
    const F &f
)
{
  {
    float scale = M_PI/180;

    forEachXYZValue(transform_state.rotation, solve_flags.rotation,
      [&](auto &value, bool solve_flag){
        f(value, solve_flag, scale);
      }
    );
  }
  {
    float scale = 1;

    forEachXYZValue(transform_state.translation, solve_flags.translation,
      [&](auto &value, bool solve_flag){
        f(value, solve_flag, scale);
      }
    );
  }
}


template <typename SceneState, typename F>
static void forEachSceneValue(SceneState &scene_state, const F &f)
{
  for (auto body_index : indicesOf(scene_state.bodies())) {
    auto &body_state = scene_state.body(body_index);
    forEachTransformValue(body_state.transform, body_state.solve_flags, f);
  }
}


static void updateState(SceneState &scene_state, const vector<float> &variables)
{
  size_t i = 0;

  forEachSceneValue(
    scene_state,
    [&](float &value, bool solve_flag, float scale){
      updateValue(value, variables, i, 1/scale, solve_flag);
    }
  );
}


void solveScene(SceneState &scene_state)
{
  vector<float> variables;

  forEachSceneValue(
    scene_state,
    [&](const float value, bool solve_flag, float scale)
    {
      getValue(variables, value, scale, solve_flag);
    }
  );

  auto f = [&]{
    updateState(scene_state, variables);
    updateErrorsInState(scene_state);
    return sceneError(scene_state);
  };

  minimize(f, variables);
  updateState(scene_state, variables);
  updateErrorsInState(scene_state);
}
