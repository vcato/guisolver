#include "taggedvalueio.hpp"

#include <sstream>
#include "quoted.hpp"
#include "printindent.hpp"
#include "stringutil.hpp"

using std::ostream;
using std::string;
using std::istringstream;


namespace {
struct PrimaryValuePrinter {
  ostream &stream;

  void operator()(PrimaryValue::Void) const
  {
  }

  void operator()(NumericValue arg) const
  {
    stream << ": " << arg;
  }

  void operator()(const StringValue &arg) const
  {
    stream << ": " << quoted(arg);
  }

  void operator()(const EnumerationValue &arg) const
  {
    stream << ": " << arg.name;
  }
};
}


void
  printTaggedValueOn(ostream &stream,const TaggedValue &tagged_value,int indent)
{
  printIndent(stream,indent);

  stream << tagged_value.tag;
  tagged_value.value.visit(PrimaryValuePrinter{stream});

  if (tagged_value.children.empty() && !tagged_value.value.isVoid()) {
    stream << "\n";
  }
  else {
    stream << " {\n";
    int n_children = tagged_value.children.size();

    for (int i=0; i!=n_children; ++i) {
      printTaggedValueOn(stream,tagged_value.children[i],indent+1);
    }

    printIndent(stream,indent);
    stream << "}\n";
  }
}


static Optional<PrimaryValue> scanPrimaryValue(StreamParser &parser)
{
  parser.skipWhitespace();

  if (parser.stream.peek()=='"') {
    parser.stream.get();
    string value;

    for (;;) {
      int c = parser.stream.get();

      if (c=='\n' ) {
        assert(false);
      }

      if (c==EOF) {
        assert(false);
      }

      if (c=='"') {
        return PrimaryValue(value);
      }

      value.push_back(c);
    }
  }

  parser.scanWord();

  if (!parser.stream) {
    assert(false);
  }

  if (startsWith(parser.word,'"')) {
    assert(false); // shouldn't happen because we've already checked for
      // a quoted string above.
  }

  if (isdigit(parser.word[0])) {
    NumericValue value;
    istringstream value_stream(parser.word);
    value_stream >> value;

    if (!value_stream) {
      assert(false);
    }

    return PrimaryValue(value);
  }

  return PrimaryValue(PrimaryValue::Enumeration{parser.word});
}


static void scanChildrenSection(TaggedValue &tagged_value,StreamParser &parser)
{
  for (;;) {
    parser.beginLine();

    parser.scanWord();

    if (parser.word=="}") {
      parser.scanEndOfLine();
      break;
    }

    Optional<TaggedValue> maybe_child_result = scanTaggedValue(parser);

    if (!maybe_child_result) {
      return;
    }

    tagged_value.children.push_back(*maybe_child_result);
  }
}




static void scanChildren(TaggedValue &tagged_value,StreamParser &parser)
{
  parser.scanWord();

  if (parser.word!="{") {
    parser.error = "Unexpected "+parser.word;
    return;
  }

  parser.scanEndOfLine();

  scanChildrenSection(tagged_value,parser);
}


Optional<TaggedValue> scanTaggedValue(StreamParser &parser)
{
  if (endsWith(parser.word,":")) {
    string tag = withoutRight(parser.word,1);
    TaggedValue tagged_value(tag);
    Optional<PrimaryValue> maybe_value = scanPrimaryValue(parser);

    if (!maybe_value) {
      assert(false);
    }

    tagged_value.value = *maybe_value;

    parser.skipWhitespace();

    if (parser.stream.peek()=='{') {
      scanChildren(tagged_value,parser);

      if (parser.hadError()) {
        assert(false);
      }
    }

    return tagged_value;
  }

  string tag = parser.word;
  TaggedValue tagged_value(tag);

  if (parser.stream.peek()=='\n') {
    return tagged_value;
  }

  scanChildren(tagged_value,parser);

  if (parser.hadError()) {
    return {};
  }

  return tagged_value;
}
