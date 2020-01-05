#include "treevalues.hpp"

#include <sstream>
#include "defaultscenestate.hpp"
#include "vectorio.hpp"
#include "indicesof.hpp"
#include "contains.hpp"
#include "faketreewidget.hpp"
#include "checktree.hpp"

using std::string;
using std::ostringstream;
using std::ostream;
using std::cerr;
using LabelText = FakeTreeItem::LabelText;


static BodyIndex
createBodyIn(SceneState &scene_state, Optional<BodyIndex> maybe_parent_index)
{
  BodyIndex body_index = scene_state.createBody(maybe_parent_index);
  scene_state.body(body_index).addBox();
  setAll(scene_state.body(body_index).solve_flags, true);
  return body_index;
}


static BodyIndex createGlobalBodyIn(SceneState &scene_state)
{
  return createBodyIn(scene_state, /*maybe_parent_index*/{});
}


template <typename F>
static void
forEachTreeItemPath(const FakeTreeItem &item, const TreePath &path, const F &f)
{
  f(path);

  for (TreeItemIndex child_index: indicesOf(item.children)) {
    forEachTreeItemPath(
      item.children[child_index],
      childPath(path, child_index),
      f
    );
  }
}


template <typename F>
static void
forEachItemInTreeWithNumericValue(const FakeTreeWidget &tree_widget, const F &f)
{
  forEachTreeItemPath(
    tree_widget.root_item,
    /*path*/{},
    [&](const TreePath &path){
      const FakeTreeItem &item = tree_widget.item(path);

      if (item.maybe_numeric_value) {
        f(path, *item.maybe_numeric_value);
      }
    }
  );
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
static void showTreePaths(const string &name, const TreePaths &tree_paths)
{
  cerr << name << ":\n";
  int indent_level = 1;
  showPaths(tree_paths.root_item, cerr, indent_level);
}
#endif


#if 0
static void showTree(const string &name, const FakeTreeWidget &tree)
{
  cerr << name << ":\n";
  int indent_level = 1;
  showItem(tree.root_item, cerr, indent_level);
}
#endif


static void testAddingMarker()
{
  SceneState state = defaultSceneState();
  FakeTreeWidget tree_widget;
  TreePaths tree_paths = fillTree(tree_widget, state);
  MarkerIndex marker_index = state.createMarker(Optional<BodyIndex>{});
  state.marker(marker_index).position = {1.5, 2.5, 3.5};
  createMarkerInTree(tree_widget, tree_paths, state, marker_index);

  assert(
    tree_widget.item(
      tree_paths.marker(marker_index).position.x
    ).maybe_numeric_value == 1.5
  );

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


static void testChangingNumericValues()
{
  SceneState test_state;
  createBodyIn(test_state, /*parent*/{});
  FakeTreeWidget tree_widget;
  TreePaths tree_paths = fillTree(tree_widget, test_state);

  auto adjust_item_value_function =
    [&](TreePath path, NumericValue value){
      tree_widget.setItemNumericValue(path, value + 1);

      bool value_was_changed =
        setSceneStateNumericValue(test_state, path, value, tree_paths);

      assert(value_was_changed);
    };

  forEachItemInTreeWithNumericValue(tree_widget, adjust_item_value_function);
  checkTree(tree_widget, tree_paths, test_state);
}


static void testRemoveBodyFromTree()
{
  FakeTreeWidget tree_widget;
  SceneState scene_state;
  BodyIndex body1_index = scene_state.createBody();
  BodyIndex body2_index = scene_state.createBody();
  BodyIndex body3_index = scene_state.createBody(/*parent*/body1_index);
  /*MarkerIndex marker1_index =*/ scene_state.createMarker(body2_index);

  TreePaths tree_paths = fillTree(tree_widget, scene_state);

  removeBodyFromTree(
    tree_widget,
    tree_paths,
    scene_state,
    body3_index
  );

  scene_state.removeBody(body3_index);

  checkTree(tree_widget, tree_paths, scene_state);
}


static void testAddingASecondBox()
{
  FakeTreeWidget tree_widget;
  SceneState scene_state;
  BodyIndex body_index = scene_state.createBody();
  scene_state.body(body_index).addBox();
  TreePaths tree_paths = fillTree(tree_widget, scene_state);
  BoxIndex box2_index = scene_state.body(body_index).addBox();
  createBoxInTree(tree_widget, tree_paths, scene_state, body_index, box2_index);
  checkTree(tree_widget, tree_paths, scene_state);
}


static void testInsertingABox()
{
  FakeTreeWidget tree_widget;
  SceneState scene_state;
  BodyIndex body_index = scene_state.createBody();
  scene_state.createMarker(body_index);
  TreePaths tree_paths = fillTree(tree_widget, scene_state);
  BoxIndex box_index = scene_state.body(body_index).addBox();
  createBoxInTree(tree_widget, tree_paths, scene_state, body_index, box_index);
  checkTree(tree_widget, tree_paths, scene_state);
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
  testChangingNumericValues();
  testRemoveBodyFromTree();
  testAddingASecondBox();
  testInsertingABox();
}
