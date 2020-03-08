#ifndef PRIMARYVALUE_HPP_
#define PRIMARYVALUE_HPP_

#include <cassert>
#include "numericvalue.hpp"
#include "stringvalue.hpp"
#include "enumerationvalue.hpp"
#include "basicvariant.hpp"


struct PrimaryValuePolicy {
  struct Void {
    bool operator==(const Void &) const { return true; }
  };

  using Enumeration = EnumerationValue;

  PrimaryValuePolicy()
  : PrimaryValuePolicy(Void{})
  {
  }

  PrimaryValuePolicy(Void)
  : _type(void_type)
  {
    createObject(_possible_values.void_value,Void{});
  }

  PrimaryValuePolicy(NumericValue arg)
  : _type(numeric_type)
  {
    createObject(_possible_values.numeric_value,arg);
  }

  PrimaryValuePolicy(StringValue arg)
  : _type(string_type)
  {
    createObject(_possible_values.string_value,arg);
  }

  PrimaryValuePolicy(Enumeration arg)
  : _type(enumeration_type)
  {
    createObject(_possible_values.enumeration_value,arg);
  }

  bool isVoid() const { return _type==void_type; }

  const std::string *maybeString() const
  {
    if (_type == string_type) {
      return &_possible_values.string_value;
    }
    else {
      return nullptr;
    }
  }

  const Enumeration *maybeEnumeration() const
  {
    if (_type == enumeration_type) {
      return &_possible_values.enumeration_value;
    }
    else {
      return nullptr;
    }
  }

  const NumericValue *maybeNumeric() const
  {
    if (_type == numeric_type) {
      return &_possible_values.numeric_value;
    }
    else {
      return nullptr;
    }
  }

  std::string typeName() const
  {
    switch (_type) {
      case void_type: return "void";
      case numeric_type: return "numeric";
      case enumeration_type: return "enumeration";
      case string_type: return "string";
    }

    assert(false);

    return "";
  }

  enum Type {
    void_type,
    numeric_type,
    enumeration_type,
    string_type
  };

  protected:
    PrimaryValuePolicy(NoInitTag)
    {
    }

    union PossibleValues {
      Void void_value;
      NumericValue numeric_value;
      Enumeration enumeration_value;
      StringValue string_value;

      PossibleValues() {}
      ~PossibleValues() {}
    };

    template <typename V>
    static auto withMemberPtrFor(Type t,const V& v)
    {
      switch (t) {
        case void_type:  return v(&PossibleValues::void_value);
        case numeric_type: return v(&PossibleValues::numeric_value);
        case enumeration_type: return v(&PossibleValues::enumeration_value);
        case string_type: return v(&PossibleValues::string_value);
      }

      assert(false);
    }

    Type _type;
    PossibleValues _possible_values;
};


using PrimaryValue = BasicVariant<PrimaryValuePolicy>;


#endif /* PRIMARYVALUE_HPP_ */
