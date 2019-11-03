#include "scenesolver.hpp"

#include "sceneerror.hpp"
#include "vec3.hpp"
#include "maketransform.hpp"
#include "optimize.hpp"
#include "eigenconv.hpp"
#include "rotationvector.hpp"


static Eigen::Matrix3f makeRotation(float x,float y,float z)
{
  return makeRotation(Vec3{x,y,z});
}


static vector<float> extractTranslationVariables(const Eigen::Vector3f &t)
{
  float x = t.x();
  float y = t.y();
  float z = t.z();
  return {x,y,z};
}


static vector<float> concat(const vector<float> &a,const vector<float> &b)
{
  auto result = a;
  result.insert(result.end(),b.begin(),b.end());
  return result;
}


static vector<float> extractRotationVariables(const Eigen::Matrix3f &r)
{
  Vec3 v = rotationVector(r);
  float x = v.x;
  float y = v.y;
  float z = v.z;
  return {x,y,z};
}


static vector<float> extractVariables(const Transform &t)
{
  // Extract rotation variables
  vector<float> rotation_variables =
    extractRotationVariables(t.rotation());

  vector<float> translation_variables =
    extractTranslationVariables(t.translation());

  // Extract translation variables.
  return concat(rotation_variables, translation_variables);
}


static Transform makeTransform(const vector<float> &variables)
{
  float rx = variables[0];
  float ry = variables[1];
  float rz = variables[2];
  float tx = variables[3];
  float ty = variables[4];
  float tz = variables[5];
  return Eigen::Translation3f(tx,ty,tz) * makeRotation(rx,ry,rz);
}


void solveBoxPosition(SceneState &scene_state)
{
  // Extract variables from transform
  vector<float> variables = extractVariables(scene_state.box_global);

  auto f = [&]{
    scene_state.box_global = makeTransform(variables);
    updateErrorsInState(scene_state);
    return sceneError(scene_state);
  };

  // minimize sceneError over the variables.
  minimize(f, variables);

  // return resulting transform.
  scene_state.box_global = makeTransform(variables);
}
