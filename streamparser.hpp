#ifndef STREAMPARSER_HPP_
#define STREAMPARSER_HPP_

#include <iostream>


struct StreamParser {
  std::istream &stream;
  std::string word;
  std::string error;
  int line_number = 1;

  StreamParser(std::istream &stream_arg);

  void setError(const std::string &arg) { error = arg; }
  void scanWord();
  bool hadError() const { return !error.empty(); }
  void scanEndOfLine();
  void skipWhitespace();
  void beginLine();
};


#endif /* STREAMPARSER_HPP_ */
