#include "streamparser.hpp"

#include <cassert>


StreamParser::StreamParser(std::istream &stream_arg)
: stream(stream_arg)
{
  beginLine();
}


void StreamParser::beginLine()
{
  for (;;) {
    skipWhitespace();

    if (stream.peek()=='\n') {
      scanEndOfLine();
    }
    else {
      break;
    }
  }
}


void StreamParser::scanWord()
{
  skipWhitespace();

  if (stream.peek()=='\n') {
    // Caller didn't check for end of line before calling this.
    assert(false);
  }

  stream >> word;
}


void StreamParser::scanEndOfLine()
{
  for (;;) {
    int c = stream.get();
    if (c==EOF || c=='\n') {
      if (c=='\n') {
        ++line_number;
      }
      break;
    }
  }
}


void StreamParser::skipWhitespace()
{
  for (;;) {
    if (stream.peek()==' ') {
      stream.get();
    }
    else {
      break;
    }
  }
}
