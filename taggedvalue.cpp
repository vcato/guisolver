#include "taggedvalue.hpp"

#include "taggedvalueio.hpp"

using std::cerr;


const TaggedValue *
findChild(const TaggedValue &tagged_value, const TaggedValue::Tag &tag)
{
  for (auto &child : tagged_value.children) {
    if (child.tag == tag) {
      return &child;
    }
  }

  return nullptr;
}


Optional<NumericValue>
findNumericValue(
  const TaggedValue &tagged_value,
  const TaggedValue::Tag &child_name
)
{
  const TaggedValue *x_ptr = findChild(tagged_value, child_name);

  if (!x_ptr) {
    return {};
  }

  const NumericValue *numeric_ptr = x_ptr->value.maybeNumeric();

  if (numeric_ptr) {
    return *numeric_ptr;
  }

  return {};
}


Optional<bool>
findBoolValue(
  const TaggedValue &tagged_value,
  const TaggedValue::Tag &child_name
)
{
  const TaggedValue *x_ptr = findChild(tagged_value, child_name);

  if (!x_ptr) {
    return {};
  }

  const EnumerationValue *value_ptr = x_ptr->value.maybeEnumeration();

  if (!value_ptr) {
    return {};
  }

  const EnumerationValue &x = *value_ptr;

  if (x.name == "false") {
    return false;
  }
  else if (x.name == "true") {
    return true;
  }
  else {
    assert(false); // not implemented
  }
}


Optional<StringValue>
  findStringValue(
    const TaggedValue &tagged_value,
    const TaggedValue::Tag &child_name
  )
{
  const TaggedValue *x_ptr = findChild(tagged_value, child_name);

  if (!x_ptr) {
    return {};
  }

  const StringValue *string_ptr = x_ptr->value.maybeString();

  if (string_ptr) {
    return *string_ptr;
  }
  else {
    return {};
  }
}
