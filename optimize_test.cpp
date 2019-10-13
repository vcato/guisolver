#include "optimize.hpp"

#include <cassert>
#include <iostream>
#include <cmath>

using std::vector;
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
  assert(fabs(variables[0]-2) < 1e-6);
  assert(result == 1);
}
