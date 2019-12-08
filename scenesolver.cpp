#include "scenesolver.hpp"

#include "sceneerror.hpp"
#include "vec3.hpp"
#include "maketransform.hpp"
#include "optimize.hpp"
#include "eigenconv.hpp"
#include "rotationvector.hpp"
#include "transformstate.hpp"

static vector<float> variables(const Vec3 &t)
{
  float x = t.x;
  float y = t.y;
  float z = t.z;
  return {x,y,z};
}


static vector<float> concat(const vector<float> &a,const vector<float> &b)
{
  auto result = a;
  result.insert(result.end(),b.begin(),b.end());
  return result;
}


static vector<float> extractVariables(const TransformState &t)
{
  vector<float> rotation_variables = variables(rotationValuesRad(t));
  vector<float> translation_variables = variables(translationValues(t));
  return concat(rotation_variables, translation_variables);
}


static TransformState makeTransform(const vector<float> &variables)
{
  float rx = variables[0];
  float ry = variables[1];
  float rz = variables[2];
  float tx = variables[3];
  float ty = variables[4];
  float tz = variables[5];
  float rx2 = rx*180/M_PI;
  float ry2 = ry*180/M_PI;
  float rz2 = rz*180/M_PI;
  return {{tx, ty, tz}, {rx2, ry2, rz2}};
}


void solveBoxPosition(SceneState &scene_state)
{
  vector<float> variables = extractVariables(scene_state.box.global);

  auto f = [&]{
    scene_state.box.global = makeTransform(variables);
    updateErrorsInState(scene_state);
    return sceneError(scene_state);
  };

  minimize(f, variables);
  scene_state.box.global = makeTransform(variables);
}
