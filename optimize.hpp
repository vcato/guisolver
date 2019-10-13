#include <vector>


struct FunctionInterface {
  virtual float operator()() const = 0;
};


extern float
  minimizeImpl(const FunctionInterface &,std::vector<float> &/*variables*/);


template <typename Function>
extern float minimize(const Function &f,std::vector<float> &variables)
{
  struct WrappedFunction : FunctionInterface {
    const Function &f;
    std::vector<float> &variables;

    WrappedFunction(const Function &f_arg,std::vector<float> &variables_arg)
    : f(f_arg),
      variables(variables_arg)
    {
    }

    float operator()() const { return f(); }
  };

  return minimizeImpl(WrappedFunction(f,variables),variables);
}
