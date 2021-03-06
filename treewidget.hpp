#ifndef TREEWIDGET_HPP_
#define TREEWIDGET_HPP_

#include <string>
#include <functional>
#include "treepath.hpp"
#include "numericvalue.hpp"
#include "stringvalue.hpp"
#include "optional.hpp"
#include "vector.hpp"


struct TreeWidget {
  struct MenuItem;
  using EnumerationOptions = vector<std::string>;
  using MenuItems = vector<MenuItem>;
  using Input = std::string;

  struct LabelProperties {
    std::string text;

    LabelProperties(const std::string &text)
    : text(text)
    {
    }

    LabelProperties(const char *text)
    : text(text)
    {
    }
  };

  struct MenuItem {
    std::string label;
    std::function<void()> callback;
    Optional<bool> optional_checked_state = {};
  };

  std::function<void(const TreePath &,NumericValue)>
    numeric_item_value_changed_callback;

  std::function<void()> selection_changed_callback;

  std::function<void(const TreePath &, bool new_value)>
    bool_item_value_changed_callback;

  std::function<void(const TreePath &, int index)>
    enumeration_item_index_changed_callback;

  std::function<void(const TreePath &, const std::string &value)>
    string_item_value_changed_callback;

  std::function<vector<MenuItem>(const TreePath &)>
    context_menu_items_callback;

  std::function<
    Optional<NumericValue> (const TreePath &, const std::string &text)
  > evaluate_function;

  virtual int itemChildCount(const TreePath &parent_item) const = 0;

  virtual void
    createVoidItem(
      const TreePath &new_item_path,
      const TreeWidget::LabelProperties &label_properties
    ) = 0;

  virtual void
    createNumericItem(
      const TreePath &new_item_path,
      const LabelProperties &,
      NumericValue value,
      NumericValue minimum_value,
      NumericValue maximum_value,
      int digits_of_precision = 2
    ) = 0;

  virtual void
    createBoolItem(
      const TreePath &new_item_path,
      const LabelProperties &,
      bool value
    ) = 0;

  virtual void
    createEnumerationItem(
      const TreePath &new_item_path,
      const LabelProperties &,
      const EnumerationOptions &options,
      int value
    ) = 0;

  virtual void
    createStringItem(
      const TreePath &new_item_path,
      const LabelProperties &,
      const std::string &value
    ) = 0;

  virtual void
    setItemNumericValue(
      const TreePath &,
      NumericValue value,
      NumericValue minimum_value,
      NumericValue maximum_value
    ) = 0;

  virtual void
    setItemNumericValue(
      const TreePath &,
      NumericValue value
    ) = 0;

  virtual void setItemInput(const TreePath &, const Input &) = 0;
  virtual void setItemBoolValue(const TreePath &path, bool value) = 0;
  virtual void setItemStringValue(const TreePath &, const StringValue &) = 0;

  virtual void
    setItemLabel(const TreePath &path,const std::string &new_label) = 0;

  virtual void setItemPending(const TreePath &path,bool) = 0;

  virtual void
    setItemEnumerationValue(
      const TreePath &,
      int index,
      const EnumerationOptions &
    ) = 0;

  virtual void selectItem(const TreePath &path) = 0;
  virtual void removeItem(const TreePath &path) = 0;
  virtual Optional<TreePath> selectedItem() const = 0;
};

#endif /* TREEWIDGET_HPP_ */
