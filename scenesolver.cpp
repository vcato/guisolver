#include "scenesolver.hpp"

#include "sceneerror.hpp"
#include "vec3.hpp"
#include "optimize.hpp"
#include "eigenconv.hpp"
#include "rotationvector.hpp"
#include "transformstate.hpp"
#include "indicesof.hpp"
#include "solveflags.hpp"

using std::cerr;


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
  auto visit_component = [&](XYZComponent component){
    switch (component) {
      case XYZComponent::x:
        f(xyz.x, solve_flags.x);
        return;
      case XYZComponent::y:
        f(xyz.y, solve_flags.y);
        return;
      case XYZComponent::z:
        f(xyz.z, solve_flags.z);
        return;
    }
  };

  forEachXYZComponent(visit_component);
}


template <typename F>
static void
visitSolvableComponent(
  SceneState::XYZ &xyz_values,
  const SceneState::XYZSolveFlags &xyz_solve_flags,
  XYZComponent component,
  const F &f2
)
{
  switch (component) {
    case XYZComponent::x:
      f2(xyz_values.x, xyz_solve_flags.x);
      return;
    case XYZComponent::y:
      f2(xyz_values.y, xyz_solve_flags.y);
      return;
    case XYZComponent::z:
      f2(xyz_values.z, xyz_solve_flags.z);
      return;
  }
}


template <typename TransformState, typename F>
static void
forEachTransformValue(
  TransformState &transform_state,
  const SceneState::TransformSolveFlags &solve_flags,
  const F &f
)
{
  struct Visitor {
    TransformState &transform_state;
    const SceneState::TransformSolveFlags &solve_flags;
    const F &f;

    void visitTranslationComponent(XYZComponent component) const
    {
      float scale = 1;

      visitSolvableComponent(
        transform_state.translation,
        solve_flags.translation,
        component,
        [&](auto &value, bool solve_flag){
          f(value, solve_flag, scale);
        }
      );
    }

    void visitRotationComponent(XYZComponent component) const
    {
      float scale = M_PI/180;

      visitSolvableComponent(
        transform_state.rotation,
        solve_flags.rotation,
        component,
        [&](auto &value, bool solve_flag){
          f(value, solve_flag, scale);
        }
      );
    }

    void visitScale() const
    {
      f(transform_state.scale, solve_flags.scale, /*scale*/1);
    }
  };

  Visitor visitor{ transform_state, solve_flags, f };
  forEachSolvableTransformElement(visitor);
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
