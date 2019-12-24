#ifndef TAGGEDVALUE_HPP_
#define TAGGEDVALUE_HPP_

#include <string>
#include <vector>
#include "primaryvalue.hpp"
#include "optional.hpp"


struct TaggedValue {
  using Tag = std::string;
  Tag tag;
  PrimaryValue value;
  std::vector<TaggedValue> children;

  TaggedValue(const Tag &tag_arg)
  : tag(tag_arg)
  {
  }

  bool operator==(const TaggedValue &arg) const
  {
    if (tag!=arg.tag) return false;
    if (value!=arg.value) return false;
    if (children!=arg.children) return false;
    return true;
  }

  bool operator!=(const TaggedValue &arg) const
  {
    return !operator==(arg);
  }
};


extern const TaggedValue *
  findChild(const TaggedValue &tagged_value, const TaggedValue::Tag &tag);


extern Optional<NumericValue>
  findNumericValue(
    const TaggedValue &tagged_value,
    const TaggedValue::Tag &child_name
  );

extern Optional<StringValue>
  findStringValue(
    const TaggedValue &tagged_value,
    const TaggedValue::Tag &child_name
  );

#endif /* TAGGEDVALUE_HPP_ */
