#include "checktree.hpp"

#include "treevalues.hpp"

using std::cerr;
using std::string;


static void checkEqual(size_t a,size_t b)
{
  if (a != b) {
    cerr << "a: " << a << "\n";
    cerr << "b: " << b << "\n";
    assert(false);
  }
}


static void checkEqual(const string &a,const string &b)
{
  if (a != b) {
    cerr << "a: " << a << "\n";
    cerr << "b: " << b << "\n";
    assert(false);
  }
}


static void checkEqual(const FakeTreeItem &a,const FakeTreeItem &b);
static void checkEqual(const TreePaths &a,const TreePaths &b);
static void checkEqual(const TreePaths::Body &a,const TreePaths::Body &b);
static void checkEqual(const TreePaths::Marker &a,const TreePaths::Marker &b);

static void
  checkEqual(
    const TreePaths::DistanceError &a,
    const TreePaths::DistanceError &b
  );


template <typename T>
static void checkEqual(const Optional<T> &a, const Optional<T> &b)
{
  checkEqual(a.hasValue(), b.hasValue());

  if (a.hasValue()) {
    checkEqual(*a, *b);
  }
}


template <typename T>
static void checkEqual(const vector<T> &a, const vector<T> &b)
{
  if (a.size() != b.size()) {
    cerr << "a.size() = " << a.size() << "\n";
    cerr << "b.size() = " << b.size() << "\n";
    assert(false);
  }

  for (auto index : indicesOf(a)) {
    checkEqual(a[index], b[index]);
  }
}


template <typename T>
static void checkMembersEqual(const T &a,const T &b);

static void checkEqual(const TreePaths::XYZ &a,const TreePaths::XYZ &b);

static void
  checkEqual(
    const TreePaths::Body::Geometry &a,
    const TreePaths::Body::Geometry &b
  );

static void checkEqual(const FakeTreeItem &a,const FakeTreeItem &b)
{
  checkMembersEqual(a,b);
}


static void checkEqual(const FakeTreeWidget &a,const FakeTreeWidget &b)
{
  checkMembersEqual(a,b);
}


template <typename T>
static void checkMembersEqual(const T &a,const T &b)
{
  T::forEachMember([&](auto T::*member_ptr){
    checkEqual(a.*member_ptr, b.*member_ptr);
  });
}


static void
checkEqual(const TreePaths::Body &a,const TreePaths::Body &b)
{
  checkMembersEqual(a,b);
}


static void
  checkEqual(
    const TreePaths::Body::Geometry &a,
    const TreePaths::Body::Geometry &b
  )
{
  checkMembersEqual(a,b);
}


static void checkEqual(const TreePaths::XYZ &a,const TreePaths::XYZ &b)
{
  checkMembersEqual(a,b);
}


static void checkEqual(const TreePaths::Marker &a,const TreePaths::Marker &b)
{
  checkMembersEqual(a,b);
}


static void
  checkEqual(
    const TreePaths::DistanceError &a,
    const TreePaths::DistanceError &b
  )
{
  checkMembersEqual(a,b);
}


static void checkEqual(const TreePaths &a,const TreePaths &b)
{
  checkMembersEqual(a,b);
}


void
checkTree(
  const FakeTreeWidget &tree_widget,
  const TreePaths &tree_paths,
  const SceneState &state
)
{
  FakeTreeWidget recreated_tree_widget;
  TreePaths recreated_tree_paths = fillTree(recreated_tree_widget, state);
  checkEqual(recreated_tree_paths, tree_paths);
  assert(recreated_tree_widget == tree_widget);
  checkEqual(recreated_tree_widget, tree_widget);
  assert(recreated_tree_paths == tree_paths);
}