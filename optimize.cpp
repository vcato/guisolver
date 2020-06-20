#include "optimize.hpp"

#include <cassert>
#include <iostream>
#include "vector"
#include <debug/vector>

using std::cerr;
using std::string;
using std::ostream;

const float min_step = 0.001;


static bool reduceStep(float &step)
{
  if (step > min_step) {
    step /= 2;

    if (step < min_step) {
      step = min_step;
    }

    return true;
  }
  else {
    return false;;
  }
}


static float optimizeVar(const FunctionInterface &f, float &var, float error)
{
  float step = min_step;

  for (;;) {
    float old_var = var;
    float forward_value = var+step;
    float reverse_value = var-step;
    var = forward_value;
    float forward_error = f();
    var = reverse_value;
    float reverse_error = f();
    var = old_var;

    if (forward_error >= error && reverse_error >= error) {
      if (!reduceStep(step)) {
        break;
      }
    }
    else {
      if (forward_error < reverse_error) {
        var = forward_value;
        error = forward_error;
      }
      else {
        var = reverse_value;
        error = reverse_error;
      }

      step *= 2;
    }
  }

  return error;
}


float minimizeImpl(const FunctionInterface &f,vector<float> &variables)
{
  float error = f();

  for (;;) {
    float error_before_optimizing = error;

    for (auto &variable : variables) {
      error = optimizeVar(f,variable,error);
    }

    if (error == error_before_optimizing) {
      break;
    }
  }

  return error;
}
