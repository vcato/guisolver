#include "stringutil.hpp"

#include <cassert>
#include <vector>
#include <sstream>

using std::vector;
using std::string;
using std::istringstream;


bool endsWith(const string &text,const string &suffix)
{
  if (suffix.size()>text.size()) return false;
  if (string(text.end()-suffix.size(),text.end())==suffix) return true;
  return false;
}


bool startsWith(const string &text,const string &prefix)
{
  if (prefix.size()>text.size()) return false;
  if (string(text.begin(),text.begin()+prefix.size())==prefix) return true;
  return false;
}


bool startsWith(const string &text,char c)
{
  if (text.length()==0) {
    assert(false);
  }

  return text[0]==c;
}


bool contains(const string &text,const string &contents)
{
  size_t position = text.find(contents);
  if (position==text.npos) {
    return false;
  }
  return true;
}


string withoutRight(const string &text,size_t n)
{
  assert(text.length()>=n);
  return text.substr(0,text.length()-n);
}
