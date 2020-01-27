#include "checktree.hpp"

#include "treevalues.hpp"

using std::cerr;
using std::string;
using BoxPaths = TreePaths::Box;
using LinePaths = TreePaths::Line;


template <typename T>
static void checkValueEqual(const T &a, const T &b)
{
  if (a != b) {
    cerr << "a: " << a << "\n";
    cerr << "b: " << b << "\n";
    assert(false);
  }
}


static void checkEqual(int a,int b)
{
  checkValueEqual(a, b);
}


static void checkEqual(const string &a,const string &b)
{
  checkValueEqual(a, b);
}


static void checkEqual(const FakeTreeItem &a,const FakeTreeItem &b);
static void checkEqual(const TreePaths &a,const TreePaths &b);
static void checkEqual(const TreePaths::Body &a,const TreePaths::Body &b);
static void checkEqual(const TreePaths::Marker &a,const TreePaths::Marker &b);

template <typename T>
static void checkEqual(const vector<T> &a, const vector<T> &b);

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
  checkEqual(a.size(), b.size());

  for (auto index : indicesOf(a)) {
    checkEqual(a[index], b[index]);
  }
}


template <typename T>
static void checkMembersEqual(const T &a,const T &b);

template <typename Component>
static void
  checkEqual(
    const TreePaths::BasicXYZ<Component> &a,
    const TreePaths::BasicXYZ<Component> &b
  )
{
  checkMembersEqual(a,b);
}

static void checkEqual(const BoxPaths &a, const BoxPaths &b);

static void checkEqual(const FakeTreeItem &a,const FakeTreeItem &b)
{
  checkMembersEqual(a,b);
}


static void checkEqual(const FakeTreeWidget &a,const FakeTreeWidget &b)
{
  checkMembersEqual(a,b);
}


#if USE_SOLVE_CHILDREN
static void checkEqual(const TreePaths::Channel &a, const TreePaths::Channel &b)
{
  checkMembersEqual(a,b);
}
#endif


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


static void checkEqual(const BoxPaths &a, const BoxPaths &b)
{
  checkMembersEqual(a,b);
}


static void checkEqual(const LinePaths &a, const LinePaths &b)
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
  assert(recreated_tree_paths == tree_paths);
  checkEqual(recreated_tree_widget, tree_widget);
  assert(recreated_tree_widget == tree_widget);
}
