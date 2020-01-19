inline std::string
rangeText(const StringRange &range, const std::string &full_text)
{
  return full_text.substr(range.begin, range.end - range.begin);
}
