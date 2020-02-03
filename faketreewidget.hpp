#ifndef FAKETREEWIDGET_HPP_
#define FAKETREEWIDGET_HPP_

#include <string>
#include "optional.hpp"
#include "numericvalue.hpp"
#include "stringvalue.hpp"
#include "vector.hpp"
#include "treewidget.hpp"


struct FakeTreeItem {
  using LabelText = std::string;
  using ValueString = std::string;
  LabelText label_text;
  ValueString value_string;
  Optional<NumericValue> maybe_numeric_value;
  vector<FakeTreeItem> children;

  FakeTreeItem() = default;

  FakeTreeItem(LabelText label_text, std::string value_string)
  : label_text(std::move(label_text)),
    value_string(std::move(value_string))
  {
  }

  template <typename F>
  static void forEachMember(const F &f)
  {
    f(&FakeTreeItem::label_text);
    f(&FakeTreeItem::value_string);
    f(&FakeTreeItem::children);
  }

  bool operator==(const FakeTreeItem &arg) const;
};


struct FakeTreeWidget : TreeWidget {
  using Item = FakeTreeItem;
  using LabelText = Item::LabelText;

  Item root_item;
  Optional<TreePath> maybe_selected_item;

  bool operator==(const FakeTreeWidget &arg) const;

  template <typename F>
  static void forEachMember(const F &f)
  {
    f(&FakeTreeWidget::root_item);
  }

  const Item &item(const TreePath &path) const { return item(root_item, path); }
  Item &item(const TreePath &path)             { return item(root_item, path); }

  void
    setItemNumericValue(
      const TreePath &,
      NumericValue /*value*/,
      NumericValue /*minimum_value*/,
      NumericValue /*maximum_value*/
    ) override
  {
    assert(false); // not implemented
  }

  void
    setItemNumericValue(
      const TreePath &path,
      NumericValue value
    ) override
  {
    item(path).maybe_numeric_value = value;
  }

  void setItemBoolValue(const TreePath &path, bool value) override
  {
    item(path).value_string = boolValueText(value);
  }

  void setItemInput(const TreePath &, const Input &) override
  {
    assert(false); // not implemented
  }

  static FakeTreeItem::ValueString boolValueText(bool);

private:
  Item &
    createItem(
      const TreePath &new_item_path,
      const LabelProperties &label_properties,
      const std::string &value_string
    )
  {
    const LabelText &label_text = label_properties.text;
    Item &parent_item = item(parentPath(new_item_path));

    parent_item.children.emplace(
      parent_item.children.begin() + new_item_path.back(),
      Item{label_text, value_string}
    );

    return parent_item.children.back();
  }

  int itemChildCount(const TreePath &path) const override
  {
    return item(path).children.size();
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
    Item &new_item =
      createItem(
        new_item_path,
        label_properties,
        numericValueText(value, minimum_value, maximum_value)
      );

    new_item.maybe_numeric_value = value;
  }

  void
    createBoolItem(
      const TreePath &new_item_path,
      const LabelProperties &label_properties,
      bool value
    ) override
  {
    createItem(new_item_path, label_properties, boolValueText(value));
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
      const TreePath &new_item_path,
      const LabelProperties &label_properties,
      const std::string &value
    ) override
  {
    createItem(
      new_item_path,
      label_properties,
      stringValueText(value)
    );
  }

  void setItemLabel(const TreePath &path,const std::string &label) override
  {
    item(path).label_text = label;
  }

  void setItemPending(const TreePath &, bool) override
  {
  }

  void
    setItemEnumerationValue(
      const TreePath &path,
      int value,
      const EnumerationOptions &options
    ) override
  {
    FakeTreeItem &item = this->item(path);
    item.value_string = enumerationValueText(value, options);
  }

  void selectItem(const TreePath &arg) override
  {
    maybe_selected_item = arg;
  }

  void removeItem(const TreePath &path) override;

  Optional<TreePath> selectedItem() const override
  {
    return maybe_selected_item;
  }

  static std::string voidValueText() { return ""; }

  static std::string
    numericValueText(
      NumericValue value,
      NumericValue minimum_value,
      NumericValue maximum_value
    );

  static LabelText
    enumerationValueText(
      int value,
      const EnumerationOptions &options
    );

  static LabelText stringValueText(const StringValue &);

  template <typename Item>
  static Item &
    item(Item &parent_item, const TreePath &path, size_t path_index = 0)
  {
    if (path_index == path.size()) {
      return parent_item;
    }

    return item(parent_item.children[path[path_index]], path, path_index + 1);
  }

};


#endif /* FAKETREEWIDGET_HPP_ */
