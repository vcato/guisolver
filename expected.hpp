#ifndef EXPECTED_HPP_
#define EXPECTED_HPP_

template <typename Value>
struct ExpectedPolicy {
  struct Error {
    std::string message;
  };

  union PossibleValues {
    Value value;
    Error error;

    PossibleValues() { }

    ~PossibleValues()
    {
      // This value should have been destroyed at this point
    }
  };

  enum class Type { value, error };

  ExpectedPolicy(const Error &error)
  : _type(Type::error)
  {
    createObject(_possible_values.error,error);
  }

  ExpectedPolicy(const Value &value)
  : _type(Type::value)
  {
    createObject(_possible_values.value, value);
  }

  template <typename Function>
  void withMemberPtrFor(Type type,const Function &f)
  {
    switch (type) {
      case Type::value: return f(&PossibleValues::value);
      case Type::error: return f(&PossibleValues::error);
    }

    assert(false);
  }

  ExpectedPolicy(NoInitTag) { }

  bool isValue() const { return _type==Type::value; }
  bool isError() const { return _type==Type::error; }

  const Error &asError() const
  {
    assert(_type==Type::error);
    return _possible_values.error;
  }

  const Value &asValue() const
  {
    assert(_type==Type::value);
    return _possible_values.value;
  }

  Type _type;
  PossibleValues _possible_values;
};


template <typename Value>
using Expected = BasicVariant<ExpectedPolicy<Value>>;


#endif /* EXPECTED_HPP_ */
