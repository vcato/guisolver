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

  if (!x_ptr->value.isNumeric()) {
    return {};
  }

  return x_ptr->value.asNumeric();
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

  if (!x_ptr->value.isEnumeration()) {
    return {};
  }

  const EnumerationValue &x = x_ptr->value.asEnumeration();

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

  if (!x_ptr->value.isString()) {
    return {};
  }

  return x_ptr->value.asString();
}
