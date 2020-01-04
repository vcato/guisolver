#include "assertnearfloat.hpp"

#include <cmath>
#include <cassert>
#include <iostream>

using std::cerr;


void assertNear(float a,float b,float tolerance)
{
  float delta = std::abs(a-b);
  bool is_near = (delta <= tolerance);

  if (!is_near) {
    cerr << "a: " << a << "\n";
    cerr << "b: " << b << "\n";
    cerr << "delta: " << delta << "\n";
  }

  assert(is_near);
}
