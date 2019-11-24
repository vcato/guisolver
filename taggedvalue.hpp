#ifndef TAGGEDVALUE_HPP_
#define TAGGEDVALUE_HPP_

#include <string>
#include <vector>
#include "primaryvalue.hpp"


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


#endif /* TAGGEDVALUE_HPP_ */
