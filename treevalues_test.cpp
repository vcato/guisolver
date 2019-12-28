#include "treevalues.hpp"

#include <sstream>
#include "defaultscenestate.hpp"
#include "streamvector.hpp"
#include "indicesof.hpp"
#include "contains.hpp"

using std::string;
using std::ostringstream;
using std::ostream;
using std::cerr;
using LabelText = std::string;


namespace {
struct FakeTreeItem {
  LabelText label_text;
  string value_string;
  vector<FakeTreeItem> children;

  template <typename F>
  static void forEachMember(const F &f)
  {
    f(&FakeTreeItem::label_text);
    f(&FakeTreeItem::value_string);
    f(&FakeTreeItem::children);
  }

  bool operator==(const FakeTreeItem &arg) const
  {
    return isEqual(*this, arg);
  }
};
}


namespace {
struct FakeTreeWidget : TreeWidget {
  using Item = FakeTreeItem;

  static string voidValueText() { return ""; }

  static string
    numericValueText(
      NumericValue value,
      NumericValue minimum_value,
      NumericValue maximum_value
    )
  {
    ostringstream stream;

    stream <<
      ", value=" << value <<
      ", min=" << minimum_value <<
      ", max=" << maximum_value;

    return stream.str();
  }

  static LabelText
    enumerationValueText(
      int value,
      const EnumerationOptions &options
    )
  {
    ostringstream stream;

    stream <<
      ", value=" << value <<
      ", options=" << options;

    return stream.str();
  }

  static Item &
    item(Item &parent_item,const TreePath &path, size_t path_index = 0)
  {
    if (path_index == path.size()) {
      return parent_item;
    }

    return item(parent_item.children[path[path_index]], path, path_index + 1);
  }

  Item root_item;

  void
    createItem(
      const TreePath &new_item_path,
      const LabelProperties &label_properties,
      const string &value_string
    )
  {
    const LabelText &label_text = label_properties.text;
    Item &parent_item = item(root_item, parentPath(new_item_path));

    parent_item.children.emplace(
      parent_item.children.begin() + new_item_path.back(),
      Item{label_text, value_string, {}}
    );
  }

  void
    createVoidItem(
      const TreePath &new_item_path,
      const TreeWidget::LabelProperties &label_properties
    ) override
  {
    createItem(new_item_path, label_properties, voidValueText());
  }

  void
    createNumericItem(
      const TreePath &new_item_path,
      const LabelProperties &label_properties,
      NumericValue value,
      NumericValue minimum_value,
      NumericValue maximum_value,
      int /*digits_of_precision*/
    ) override
  {
    createItem(
      new_item_path,
      label_properties,
      numericValueText(value, minimum_value, maximum_value)
    );
  }

  void
    createEnumerationItem(
      const TreePath &new_item_path,
      const LabelProperties &label_properties,
      const EnumerationOptions &options,
      int value
    ) override
  {
    createItem(
      new_item_path,
      label_properties,
      enumerationValueText(value, options)
    );
  }

  void
    createStringItem(
      const TreePath &/*new_item_path*/,
      const LabelProperties &,
      const std::string &/*value*/
    ) override
  {
    assert(false); // not implemented
  }

  void
    setItemNumericValue(
      const TreePath &,
      NumericValue /*value*/,
      NumericValue /*minimum_value*/,
      NumericValue /*maximum_value*/
    )
  {
    assert(false); // not implemented
  }

  void
    setItemNumericValue(
      const TreePath &,
      NumericValue
    ) override
  {
    assert(false); // not implemented
  }

  void setItemLabel(const TreePath &,const std::string &) override
  {
    assert(false); // not implemented
  }

  void
    setItemEnumerationValue(
      const TreePath &path,
      int value,
      const EnumerationOptions &options
    ) override
  {
    FakeTreeItem &item = this->item(root_item, path);
    item.value_string = enumerationValueText(value, options);
  }

  void selectItem(const TreePath &) override
  {
    assert(false); // not implemented
  }

  void removeItem(const TreePath &path) override
  {
    Item &parent_item = item(root_item, parentPath(path));
    removeIndexFrom(parent_item.children, path.back());
  }

  Optional<TreePath> selectedItem() const override
  {
    assert(false); // not implemented
  }

  template <typename F>
  static void forEachMember(const F &f)
  {
    f(&FakeTreeWidget::root_item);
  }

  bool operator==(const FakeTreeWidget &arg) const
  {
    return isEqual(*this, arg);
  }
};
}


static BodyIndex
createBodyIn(SceneState &scene_state, Optional<BodyIndex> maybe_parent_index)
{
  return scene_state.createBody(maybe_parent_index);
}


static BodyIndex createGlobalBodyIn(SceneState &scene_state)
{
  return createBodyIn(scene_state, /*maybe_parent_index*/{});
}


static void testRemovingDistanceError()
{
  SceneState state = defaultSceneState();
  FakeTreeWidget tree_widget;
  TreePaths tree_paths = fillTree(tree_widget, state);
  int distance_error_to_remove_index = 2;

  removeDistanceErrorFromTree(
    distance_error_to_remove_index,
    tree_paths,
    tree_widget
  );

  state.removeDistanceError(distance_error_to_remove_index);
  FakeTreeWidget recreated_tree_widget;
  TreePaths new_tree_paths = fillTree(recreated_tree_widget, state);

  assert(recreated_tree_widget == tree_widget);
  assert(new_tree_paths == tree_paths);
}


static void indent(ostream &stream, int indent_level)
{
  for (int i=0; i!=indent_level; ++i) {
    stream << "  ";
  }
}


static void
  showItem(const FakeTreeItem &item, ostream &stream, int &indent_level)
{
  indent(stream, indent_level);
  stream << item.label_text << ": " << item.value_string << "\n";
  ++indent_level;

  for (auto &child_item : item.children) {
    showItem(child_item, stream, indent_level);
  }

  --indent_level;
}


#if 0
static void showTree(const string &name, const FakeTreeWidget &tree)
{
  cerr << name << ":\n";
  int indent_level = 1;
  showItem(tree.root_item, cerr, indent_level);
}
#endif


#if 0
static void showTreePaths(const string &name, const TreePaths &tree_paths)
{
  cerr << name << ":\n";
  int indent_level = 1;
  showPaths(tree_paths.root_item, cerr, indent_level);
}
#endif


template <typename T>
static void checkMembersEqual(const T &a,const T &b);

static void checkEqual(size_t a,size_t b)
{
  if (a != b) {
    cerr << "a: " << a << "\n";
    cerr << "b: " << b << "\n";
    assert(false);
  }
}


static void checkEqual(const FakeTreeWidget &a,const FakeTreeWidget &b)
{
  checkMembersEqual(a,b);
}


static void checkEqual(const FakeTreeItem &a,const FakeTreeItem &b)
{
  checkMembersEqual(a,b);
}


template <typename T>
static void checkEqual(const vector<T> &a,const vector<T> &b)
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


static void
checkEqual(const TreePaths::Body &a,const TreePaths::Body &b)
{
  checkMembersEqual(a,b);
  checkEqual(a.next_marker_path, b.next_marker_path);
  checkEqual(a.next_body_path, b.next_body_path);
}


static void
  checkEqual(
    const TreePaths::Body::Geometry &a,
    const TreePaths::Body::Geometry &b
  )
{
  checkMembersEqual(a,b);
}


static void
  checkEqual(const TreePaths::XYZ &a,const TreePaths::XYZ &b)
{
  checkMembersEqual(a,b);
}


static void
  checkEqual(const TreePaths::Marker &a,const TreePaths::Marker &b)
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


static void checkEqual(const string &a,const string &b)
{
  if (a != b) {
    cerr << "a: " << a << "\n";
    cerr << "b: " << b << "\n";
    assert(false);
  }
}


static void checkEqual(const TreePaths &a,const TreePaths &b)
{
  checkMembersEqual(a,b);
  checkEqual(a.next_distance_error_path, b.next_distance_error_path);
  checkEqual(a.next_scene_marker_path, b.next_scene_marker_path);
  checkEqual(a.next_scene_body_path, b.next_scene_body_path);
}


template <typename T>
static void checkMembersEqual(const T &a,const T &b)
{
  T::forEachMember([&](auto T::*member_ptr){
    checkEqual(a.*member_ptr, b.*member_ptr);
  });
}


static void
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


static void testAddingMarker()
{
  SceneState state = defaultSceneState();
  FakeTreeWidget tree_widget;
  TreePaths tree_paths = fillTree(tree_widget, state);
  MarkerIndex marker_index = createMarkerInState(state, /*maybe_body_index*/{});
  createMarkerInTree(tree_widget, tree_paths, state, marker_index);
  updateTreeDistanceErrorMarkerOptions(tree_widget, tree_paths, state);
  checkTree(tree_widget, tree_paths, state);
}


static void testRemovingMarker(const string &name)
{
  SceneState state = defaultSceneState();
  FakeTreeWidget tree_widget;
  TreePaths tree_paths = fillTree(tree_widget, state);
  MarkerIndex marker_index = findIndex(markerNames(state), name);
  state.removeMarker(marker_index);
  removeMarkerFromTree(marker_index, tree_paths, tree_widget);
  updateTreeDistanceErrorMarkerOptions(tree_widget, tree_paths, state);
  checkTree(tree_widget, tree_paths, state);
}


static void testRemovingAGlobalMarker()
{
  testRemovingMarker("global1");
}


static void testRemovingALocalMarker()
{
  testRemovingMarker("local1");
}


static void testAddingASceneBody()
{
  SceneState state = defaultSceneState();
  FakeTreeWidget tree_widget;
  TreePaths tree_paths = fillTree(tree_widget, state);
  BodyIndex body_index = createGlobalBodyIn(state);
  createBodyInTree(tree_widget, tree_paths, state, body_index);
  checkTree(tree_widget, tree_paths, state);
}


static void testAddingAChildBody()
{
  SceneState state = defaultSceneState();
  FakeTreeWidget tree_widget;
  TreePaths tree_paths = fillTree(tree_widget, state);

  BodyIndex body_index =
    createBodyIn(state, /*parent_body_index*/boxBodyIndex());

  createBodyInTree(tree_widget, tree_paths, state, body_index);
  checkTree(tree_widget, tree_paths, state);
}


static void testRemovingAGlobalBody()
{
  SceneState state;
  FakeTreeWidget tree_widget;
  BodyIndex body1_index = createGlobalBodyIn(state);
  createGlobalBodyIn(state);
  state.createMarker("global");
  TreePaths tree_paths = fillTree(tree_widget, state);
  removeBodyFromTree(tree_widget, tree_paths, state, body1_index);
  state.removeBody(body1_index);
  checkTree(tree_widget, tree_paths, state);
}


static void testRemovingAChildBody()
{
  SceneState state;
  FakeTreeWidget tree_widget;
  BodyIndex parent_body_index = createGlobalBodyIn(state);
  BodyIndex body1_index = createBodyIn(state, parent_body_index);
  createBodyIn(state, parent_body_index);
  MarkerIndex marker_index = state.createMarker("global");
  state.marker(marker_index).maybe_body_index = parent_body_index;
  TreePaths tree_paths = fillTree(tree_widget, state);
  removeBodyFromTree(tree_widget, tree_paths, state, body1_index);
  state.removeBody(body1_index);
  checkTree(tree_widget, tree_paths, state);
}


int main()
{
  testRemovingDistanceError();
  testAddingMarker();
  testRemovingAGlobalMarker();
  testRemovingALocalMarker();
  testAddingASceneBody();
  testAddingAChildBody();
  testRemovingAGlobalBody();
  testRemovingAChildBody();
}
