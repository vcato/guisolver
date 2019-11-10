#include "treevalues.hpp"

#include "defaultscenestate.hpp"
#include "streamvector.hpp"

using std::string;
using std::ostringstream;
using LabelText = std::string;

namespace {
struct FakeTreeWidget : TreeWidget {
  struct Item {
    LabelText label;
    vector<Item> children;

    bool operator==(const Item &arg) const
    {
      return label == arg.label && children == arg.children;
    }
  };

  static LabelText voidLabelText(const LabelProperties &label_properties)
  {
    return label_properties.text;
  }

  static LabelText
    numericLabelText(
      const LabelProperties &label_properties,
      NumericValue value,
      NumericValue minimum_value,
      NumericValue maximum_value
    )
  {
    ostringstream stream;

    stream <<
      label_properties.text <<
      ", value=" << value <<
      ", min=" << minimum_value <<
      ", max=" << maximum_value;

    return stream.str();
  }

  static LabelText
    enumerationLabelText(
      const LabelProperties &label_properties,
      int value,
      const EnumerationOptions &options
    )
  {
    ostringstream stream;

    stream <<
      label_properties.text <<
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

  void createItem(const TreePath &new_item_path, const LabelText &label_text)
  {
    Item &parent_item = item(root_item, parentPath(new_item_path));

    parent_item.children.emplace(
      parent_item.children.begin() + new_item_path.back(),
      Item{label_text,{}}
    );
  }

  void
    createVoidItem(
      const TreePath &new_item_path,
      const TreeWidget::LabelProperties &label_properties
    ) override
  {
    createItem(new_item_path, voidLabelText(label_properties));
  }

  void
    createNumericItem(
      const TreePath &new_item_path,
      const LabelProperties &label_properties,
      NumericValue value,
      NumericValue minimum_value,
      NumericValue maximum_value
    ) override
  {
    createItem(
      new_item_path,
      numericLabelText(label_properties, value, minimum_value, maximum_value)
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
      enumerationLabelText(label_properties, value, options)
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

  bool operator==(const FakeTreeWidget &arg) const
  {
    return root_item == arg.root_item;
  }
};
}


int main()
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
  FakeTreeWidget new_tree_widget;
  TreePaths new_tree_paths = fillTree(new_tree_widget, state);

  assert(new_tree_widget == tree_widget);
  assert(new_tree_paths == tree_paths);
}
