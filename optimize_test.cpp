#include "optimize.hpp"

#include <cassert>
#include <iostream>
#include <cmath>

using std::cerr;
using std::fabs;


int main()
{
  vector<float> variables(1,0);

  auto f = [&]{
    float x = variables[0];
    float result = (x-2)*(x-2) + 1;
    return result;
  };

  float result = minimize(f,variables);
  float delta = fabs(variables[0]-2);
  float tolerance = 3e-4;

  if (delta > tolerance) {
    cerr << "delta: " << delta << "\n";
    cerr << "tolerance: " << tolerance << "\n";
  }

  assert(delta <= tolerance);
  assert(result == 1);
}
