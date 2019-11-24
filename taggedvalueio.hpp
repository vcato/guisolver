#ifndef TAGGEDVALUEIO_HPP_
#define TAGGEDVALUEIO_HPP_

#include <iostream>
#include "taggedvalue.hpp"
#include "optional.hpp"
#include "streamparser.hpp"
#include "expected.hpp"

using ScanTaggedValueResult = Expected<TaggedValue>;


extern void
  printTaggedValueOn(
    std::ostream &stream,
    const TaggedValue &tagged_value,
    int indent
  );

extern Optional<TaggedValue> scanTaggedValue(StreamParser &);
extern ScanTaggedValueResult scanTaggedValueFrom(std::istream &);


#endif /* TAGGEDVALUEIO_HPP_ */
