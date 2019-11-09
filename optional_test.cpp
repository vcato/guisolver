#include "optional.hpp"

#include <string>
#include <memory>

using std::string;
using std::unique_ptr;


static void testOptionalInt()
{
  Optional<int> v;

  assert(!v);
  assert(!v.hasValue());
  assert(!(v==5));

  v = 5;

  assert(v);
  assert(*v==5);
  assert(v==5);

  v.emplace();

  assert(v==0);

  v = Optional<int>(1);

  assert(v==1);

  v = Optional<int>();

  assert(!v);
}


static void testOptionalString()
{
  Optional<string> s("test");
  assert(*s=="test");
}


static void testEmplace()
{
  Optional<string> s;
  s.emplace();
  assert(s == string(""));
}


static void testMovingWithNoncopyableValue()
{
  using NonCopyable = std::unique_ptr<int>;

  Optional<NonCopyable> a;
  Optional<NonCopyable> b = std::move(a);
}


static void testCopy()
{
  {
    Optional<int> a;
    Optional<int> b = a;
    assert(!a && !b);
  }
  {
    Optional<int> a = 5;
    Optional<int> b = a;
    assert(a==5 && b==5);
  }
}


static void testAssign()
{
  Optional<int> a = 5;
  a = Optional<int>{};
  assert(!a);
}


int main()
{
  testOptionalInt();
  testOptionalString();
  testEmplace();
  testMovingWithNoncopyableValue();
  testCopy();
  testAssign();
}
