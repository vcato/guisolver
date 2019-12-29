#include "vector.hpp"
#include "functioninterface.hpp"


extern float
  minimizeImpl(const FunctionInterface &,vector<float> &/*variables*/);


template <typename Function>
extern float minimize(const Function &f,vector<float> &variables)
{
  struct WrappedFunction : FunctionInterface {
    const Function &f;
    vector<float> &variables;

    WrappedFunction(const Function &f_arg,vector<float> &variables_arg)
    : f(f_arg),
      variables(variables_arg)
    {
    }

    float operator()() const { return f(); }
  };

  return minimizeImpl(WrappedFunction(f,variables),variables);
}
